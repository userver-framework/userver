#include <userver/ugrpc/server/generic_service_base.hpp>

#include <userver/engine/sleep.hpp>
#include <userver/ugrpc/byte_buffer_utils.hpp>
#include <userver/ugrpc/client/exceptions.hpp>
#include <userver/ugrpc/tests/service_fixtures.hpp>
#include <userver/utest/log_capture_fixture.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/text_light.hpp>

#include <tests/unit_test_client.usrv.pb.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

/// [sample]
constexpr std::string_view kSayHelloCallName = "sample.ugrpc.UnitTestService/SayHello";

class SampleGenericService final : public ugrpc::server::GenericServiceBase {
public:
    GenericResult Handle(GenericCallContext& context, GenericReaderWriter& stream) override {
        EXPECT_EQ(context.GetCallName(), kSayHelloCallName);

        grpc::ByteBuffer request_bytes;
        EXPECT_TRUE(stream.Read(request_bytes));

        sample::ugrpc::GreetingRequest request;
        if (!ugrpc::ParseFromByteBuffer(std::move(request_bytes), request)) {
            return grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "Failed to parse request"};
        }

        sample::ugrpc::GreetingResponse response;
        response.set_name("Hello " + request.name());

        return ugrpc::SerializeToByteBuffer(response);
    }
};
/// [sample]

void PerformGenericUnaryCall(const sample::ugrpc::UnitTestServiceClient& client) {
    sample::ugrpc::GreetingRequest out;
    out.set_name("generic");

    const auto in = client.SayHello(out);

    EXPECT_EQ(in.name(), "Hello generic");
}

class RealCallNameGenericService final : public ugrpc::server::GenericServiceBase {
public:
    GenericResult Handle(GenericCallContext& context, GenericReaderWriter& /*stream*/) override {
        context.SetMetricsCallName(context.GetCallName());
        return grpc::Status{grpc::StatusCode::UNAUTHENTICATED, "To avoid message parsing bureaucracy"};
    }
};

}  // namespace

using GenericServiceTest =
    ugrpc::tests::ServiceWithClientFixture<SampleGenericService, sample::ugrpc::UnitTestServiceClient>;

UTEST_F(GenericServiceTest, UnaryCall) { PerformGenericUnaryCall(GetClient()); }

using RealCallNameGenericServiceTest =
    ugrpc::tests::ServiceWithClientFixture<RealCallNameGenericService, sample::ugrpc::UnitTestServiceClient>;

UTEST_F(RealCallNameGenericServiceTest, MetricsRealUnsafe) {
    UEXPECT_THROW_MSG(
        PerformGenericUnaryCall(GetClient()),
        ugrpc::client::UnauthenticatedError,
        "To avoid message parsing bureaucracy"
    );

    // Server writes metrics after Finish, after the client might have returned
    // from Finish.
    GetServer().StopServing();

    {
        const auto stats = GetStatistics(
            "grpc.server.by-destination",
            {
                {"grpc_service", "sample.ugrpc.UnitTestService"},
                {"grpc_method", "SayHello"},
                {"grpc_destination", "sample.ugrpc.UnitTestService/SayHello"},
            }
        );
        EXPECT_EQ(stats.SingleMetricOptional("status", {{"grpc_code", "UNAUTHENTICATED"}}), utils::statistics::Rate{1})
            << testing::PrintToString(stats);
    }

    {
        const auto stats = GetStatistics(
            "grpc.server.by-destination",
            {
                {"grpc_service", "Generic"},
                {"grpc_method", "Generic"},
                {"grpc_destination", "Generic/Generic"},
            }
        );
        EXPECT_EQ(stats.SingleMetricOptional("rps"), utils::statistics::Rate{0}) << testing::PrintToString(stats);
    }
}

UTEST_F(GenericServiceTest, MetricsDefaultCallNameIsFake) {
    PerformGenericUnaryCall(GetClient());

    // Server writes metrics after Finish, after the client might have returned
    // from Finish.
    GetServer().StopServing();

    const auto stats = GetStatistics(
        "grpc.server.by-destination",
        {
            {"grpc_service", "Generic"},
            {"grpc_method", "Generic"},
            {"grpc_destination", "Generic/Generic"},
        }
    );
    EXPECT_EQ(stats.SingleMetricOptional("status", {{"grpc_code", "OK"}}), utils::statistics::Rate{1})
        << testing::PrintToString(stats);
}

using GenericServerLoggingTest = utest::LogCaptureFixture<GenericServiceTest>;

UTEST_F(GenericServerLoggingTest, Logs) {
    PerformGenericUnaryCall(GetClient());

    // Server writes metrics after Finish, after the client might have returned
    // from Finish.
    GetServer().StopServing();

    const auto span_log = GetSingleLog(GetLogCapture().Filter(
        "",
        {{
            {std::string_view("span_kind"), std::string_view("server")},
            {std::string_view("stopwatch_name"), std::string_view("sample.ugrpc.UnitTestService/SayHello")},
        }}
    ));
    EXPECT_EQ(span_log.GetTagOptional("stopwatch_name"), kSayHelloCallName) << span_log;
    EXPECT_EQ(span_log.GetTagOptional("rpc.system"), "grpc") << span_log;
    EXPECT_EQ(span_log.GetTagOptional("rpc.service"), "sample.ugrpc.UnitTestService") << span_log;
    EXPECT_EQ(span_log.GetTagOptional("rpc.method"), "SayHello") << span_log;
    EXPECT_EQ(span_log.GetTagOptional(tracing::kGrpcCode), "OK") << span_log;
}

USERVER_NAMESPACE_END
