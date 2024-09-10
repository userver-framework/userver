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
constexpr std::string_view kSayHelloCallName =
    "sample.ugrpc.UnitTestService/SayHello";

class SampleGenericService final : public ugrpc::server::GenericServiceBase {
 public:
  void Handle(Call& call) override {
    EXPECT_EQ(call.GetCallName(), kSayHelloCallName);

    grpc::ByteBuffer request_bytes;
    ASSERT_TRUE(call.Read(request_bytes));

    sample::ugrpc::GreetingRequest request;
    if (!ugrpc::ParseFromByteBuffer(std::move(request_bytes), request)) {
      call.FinishWithError(grpc::Status{grpc::StatusCode::INVALID_ARGUMENT,
                                        "Failed to parse request"});
      return;
    }

    sample::ugrpc::GreetingResponse response;
    response.set_name("Hello " + request.name());

    call.WriteAndFinish(ugrpc::SerializeToByteBuffer(response));
  }
};
/// [sample]

void PerformGenericUnaryCall(
    const sample::ugrpc::UnitTestServiceClient& client) {
  sample::ugrpc::GreetingRequest out;
  out.set_name("generic");

  auto call = client.SayHello(out);

  const auto in = call.Finish();
  EXPECT_EQ(in.name(), "Hello generic");
}

class RealCallNameGenericService final
    : public ugrpc::server::GenericServiceBase {
 public:
  void Handle(Call& call) override {
    call.SetMetricsCallName(call.GetCallName());
    call.FinishWithError(grpc::Status{grpc::StatusCode::UNAUTHENTICATED,
                                      "To avoid message parsing bureaucracy"});
  }
};

}  // namespace

using GenericServiceTest = ugrpc::tests::ServiceFixture<SampleGenericService>;

UTEST_F(GenericServiceTest, UnaryCall) {
  PerformGenericUnaryCall(MakeClient<sample::ugrpc::UnitTestServiceClient>());
}

using RealCallNameGenericServiceTest =
    ugrpc::tests::ServiceFixture<RealCallNameGenericService>;

UTEST_F(RealCallNameGenericServiceTest, MetricsRealUnsafe) {
  UEXPECT_THROW_MSG(PerformGenericUnaryCall(
                        MakeClient<sample::ugrpc::UnitTestServiceClient>()),
                    ugrpc::client::UnauthenticatedError,
                    "To avoid message parsing bureaucracy");

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
        });
    EXPECT_EQ(stats.SingleMetricOptional("status",
                                         {{"grpc_code", "UNAUTHENTICATED"}}),
              utils::statistics::Rate{1})
        << testing::PrintToString(stats);
  }

  {
    const auto stats =
        GetStatistics("grpc.server.by-destination",
                      {
                          {"grpc_service", "Generic"},
                          {"grpc_method", "Generic"},
                          {"grpc_destination", "Generic/Generic"},
                      });
    EXPECT_EQ(stats.SingleMetricOptional("rps"), utils::statistics::Rate{0})
        << testing::PrintToString(stats);
  }
}

UTEST_F(GenericServiceTest, MetricsDefaultCallNameIsFake) {
  PerformGenericUnaryCall(MakeClient<sample::ugrpc::UnitTestServiceClient>());

  // Server writes metrics after Finish, after the client might have returned
  // from Finish.
  GetServer().StopServing();

  const auto stats = GetStatistics("grpc.server.by-destination",
                                   {
                                       {"grpc_service", "Generic"},
                                       {"grpc_method", "Generic"},
                                       {"grpc_destination", "Generic/Generic"},
                                   });
  EXPECT_EQ(stats.SingleMetricOptional("status", {{"grpc_code", "OK"}}),
            utils::statistics::Rate{1})
      << testing::PrintToString(stats);
}

using GenericServerLoggingTest = utest::LogCaptureFixture<GenericServiceTest>;

UTEST_F(GenericServerLoggingTest, Logs) {
  PerformGenericUnaryCall(MakeClient<sample::ugrpc::UnitTestServiceClient>());

  // Server writes metrics after Finish, after the client might have returned
  // from Finish.
  GetServer().StopServing();

  const auto span_log =
      GetSingleLog(GetLogCapture().Filter([](const utest::LogRecord& record) {
        const auto span_name = record.GetTagOptional("stopwatch_name");
        return span_name && utils::text::StartsWith(*span_name, "grpc/");
      }));
  EXPECT_EQ(span_log.GetTagOptional("stopwatch_name"),
            "grpc/sample.ugrpc.UnitTestService/SayHello")
      << span_log;
  EXPECT_EQ(span_log.GetTagOptional("grpc_code"), "OK") << span_log;
}

USERVER_NAMESPACE_END
