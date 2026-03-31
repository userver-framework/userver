#include <userver/utest/utest.hpp>

#include <vector>

#include <gmock/gmock.h>

#include <userver/engine/async.hpp>
#include <userver/ugrpc/client/graceful_stream_finish.hpp>

#include <tests/unit_test_client.usrv.pb.hpp>
#include <tests/unit_test_service.usrv.pb.hpp>
#include <userver/ugrpc/tests/service_fixtures.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

class UnitTestServiceEcho final : public sample::ugrpc::UnitTestServiceBase {
public:
    ChatResult Chat(CallContext& /*context*/, ChatReaderWriter& stream) override {
        sample::ugrpc::StreamGreetingRequest request;
        while (stream.Read(request)) {
            sample::ugrpc::StreamGreetingResponse response{};
            response.set_number(request.number());
            stream.Write(std::move(response));
        }
        return grpc::Status::OK;
    }

    ReadManyResult ReadMany(
        CallContext& /*context*/,
        sample::ugrpc::StreamGreetingRequest&& /*request*/,
        ReadManyWriter& writer
    ) override {
        for (int i = 0; i < 3; ++i) {
            sample::ugrpc::StreamGreetingResponse response{};
            response.set_number(i);
            writer.Write(std::move(response));
        }
        return grpc::Status::OK;
    }

    WriteManyResult WriteMany(CallContext& /*context*/, WriteManyReader& reader) override {
        sample::ugrpc::StreamGreetingRequest request{};
        std::size_t requests = 0;
        while (reader.Read(request)) {
            requests += 1;
        }
        sample::ugrpc::StreamGreetingResponse response;
        response.set_number(requests);
        return response;
    }
};

}  // namespace

using GrpcBidirectionalStream =
    ugrpc::tests::ServiceWithClientFixture<UnitTestServiceEcho, sample::ugrpc::UnitTestServiceClient>;
using GrpcInputStream =
    ugrpc::tests::ServiceWithClientFixture<UnitTestServiceEcho, sample::ugrpc::UnitTestServiceClient>;
using GrpcOutputStream =
    ugrpc::tests::ServiceWithClientFixture<UnitTestServiceEcho, sample::ugrpc::UnitTestServiceClient>;

UTEST_F_MT(GrpcBidirectionalStream, BidirectionalStreamTest, 2) {
    constexpr std::size_t kMessagesCount = 200;

    auto stream = GetClient().Chat();

    std::vector<sample::ugrpc::StreamGreetingRequest> requests;
    requests.resize(kMessagesCount);
    std::vector<sample::ugrpc::StreamGreetingResponse> responses;

    /// [concurrent bidirectional stream]
    auto write_task = engine::AsyncNoSpan([&stream, &requests] {
        for (const auto& request : requests) {
            const bool success = stream.Write(request);
            if (!success) {
                return false;
            }
        }

        return stream.WritesDone();
    });

    sample::ugrpc::StreamGreetingResponse response;
    while (stream.Read(response)) {
        responses.push_back(std::move(response));
    }

    ASSERT_TRUE(write_task.Get());
    /// [concurrent bidirectional stream]

    ASSERT_EQ(responses.size(), kMessagesCount);

    ASSERT_FALSE(stream.Write(sample::ugrpc::StreamGreetingRequest()));
    ASSERT_FALSE(stream.WritesDone());
    UEXPECT_THROW_MSG(
        stream.WriteAndCheck(sample::ugrpc::StreamGreetingRequest()),
        ugrpc::client::RpcError,
        "'WriteAndCheck' called on a finished or closed stream"
    );
    ASSERT_FALSE(stream.Read(response));
    UEXPECT_THROW_MSG(
        [[maybe_unused]] auto _ = stream.ReadAsync(response),
        ugrpc::client::RpcError,
        "'ReadAsync' called on a finished call"
    );
}

UTEST_F(GrpcBidirectionalStream, PingPongFinishOk) {
    auto stream = GetClient().Chat();

    ASSERT_TRUE(stream.Write(sample::ugrpc::StreamGreetingRequest()));
    sample::ugrpc::StreamGreetingResponse response;
    ASSERT_TRUE(stream.Read(response));

    ASSERT_TRUE(ugrpc::client::PingPongFinish(stream));

    ASSERT_FALSE(stream.Write(sample::ugrpc::StreamGreetingRequest()));
    ASSERT_FALSE(stream.WritesDone());
    ASSERT_THROW(stream.WriteAndCheck(sample::ugrpc::StreamGreetingRequest()), ugrpc::client::RpcError);
    ASSERT_FALSE(stream.Read(response));
    ASSERT_THROW([[maybe_unused]] auto _ = stream.ReadAsync(response), ugrpc::client::RpcError);
}

UTEST_F(GrpcBidirectionalStream, PingPongFinishNoMessages) {
    auto stream = GetClient().Chat();

    ASSERT_TRUE(ugrpc::client::PingPongFinish(stream));

    ASSERT_FALSE(stream.Write(sample::ugrpc::StreamGreetingRequest()));
    ASSERT_FALSE(stream.WritesDone());
    ASSERT_THROW(stream.WriteAndCheck(sample::ugrpc::StreamGreetingRequest()), ugrpc::client::RpcError);
    sample::ugrpc::StreamGreetingResponse response;
    ASSERT_FALSE(stream.Read(response));
    ASSERT_THROW([[maybe_unused]] auto _ = stream.ReadAsync(response), ugrpc::client::RpcError);
}

UTEST_F(GrpcBidirectionalStream, PingPongFinishMoreMessages) {
    auto stream = GetClient().Chat();

    ASSERT_TRUE(stream.Write(sample::ugrpc::StreamGreetingRequest()));
    // No 'Read' here

    ASSERT_FALSE(ugrpc::client::PingPongFinish(stream));

    ASSERT_FALSE(stream.Write(sample::ugrpc::StreamGreetingRequest()));
    ASSERT_FALSE(stream.WritesDone());
    ASSERT_THROW(stream.WriteAndCheck(sample::ugrpc::StreamGreetingRequest()), ugrpc::client::RpcError);
    sample::ugrpc::StreamGreetingResponse response;
    ASSERT_FALSE(stream.Read(response));
    ASSERT_THROW([[maybe_unused]] auto _ = stream.ReadAsync(response), ugrpc::client::RpcError);
}

UTEST_F(GrpcBidirectionalStream, PingPongFinishAfterWritesDone) {
    auto stream = GetClient().Chat();

    ASSERT_TRUE(stream.WritesDone());

    ASSERT_FALSE(ugrpc::client::PingPongFinish(stream));

    ASSERT_FALSE(stream.Write(sample::ugrpc::StreamGreetingRequest()));
    ASSERT_FALSE(stream.WritesDone());
    ASSERT_THROW(stream.WriteAndCheck(sample::ugrpc::StreamGreetingRequest()), ugrpc::client::RpcError);
    sample::ugrpc::StreamGreetingResponse response;
    ASSERT_FALSE(stream.Read(response));
    ASSERT_THROW([[maybe_unused]] auto _ = stream.ReadAsync(response), ugrpc::client::RpcError);
}

UTEST_F(GrpcBidirectionalStream, BidirectionalStreamReadRemaining) {
    auto stream = GetClient().Chat();

    ASSERT_TRUE(stream.Write(sample::ugrpc::StreamGreetingRequest()));
    ASSERT_THAT(ugrpc::client::ReadRemainingAndFinish(stream), testing::Optional(1));

    ASSERT_FALSE(stream.Write(sample::ugrpc::StreamGreetingRequest()));
    ASSERT_FALSE(stream.WritesDone());
    ASSERT_THROW(stream.WriteAndCheck(sample::ugrpc::StreamGreetingRequest()), ugrpc::client::RpcError);
    sample::ugrpc::StreamGreetingResponse response;
    ASSERT_FALSE(stream.Read(response));
    ASSERT_THROW([[maybe_unused]] auto _ = stream.ReadAsync(response), ugrpc::client::RpcError);
}

UTEST_F(GrpcBidirectionalStream, BidirectionalStreamReadRemainingAfterWritesDone) {
    auto stream = GetClient().Chat();

    ASSERT_TRUE(stream.Write(sample::ugrpc::StreamGreetingRequest()));
    ASSERT_TRUE(stream.WritesDone());
    ASSERT_FALSE(ugrpc::client::ReadRemainingAndFinish(stream).has_value());

    ASSERT_FALSE(stream.Write(sample::ugrpc::StreamGreetingRequest()));
    ASSERT_FALSE(stream.WritesDone());
    ASSERT_THROW(stream.WriteAndCheck(sample::ugrpc::StreamGreetingRequest()), ugrpc::client::RpcError);
    sample::ugrpc::StreamGreetingResponse response;
    ASSERT_FALSE(stream.Read(response));
    ASSERT_THROW([[maybe_unused]] auto _ = stream.ReadAsync(response), ugrpc::client::RpcError);
}

UTEST_F(GrpcBidirectionalStream, BidirectionalStreamDestroy) {
    auto stream1 = GetClient().Chat();
    auto stream2 = GetClient().Chat();

    constexpr std::string_view kAbandoned = "abandoned-error";
    constexpr std::string_view kStatus = "status";
    const auto get_metric = [this](std::string_view name, std::vector<utils::statistics::Label> labels = {}) {
        const auto stats = GetStatistics("grpc.client.total", labels);
        return stats.SingleMetric(std::string{name}, labels).AsRate();
    };

    EXPECT_EQ(get_metric(kStatus, {{"grpc_code", "OK"}}), 0);
    EXPECT_FALSE(GetStatistics("grpc.client.total", {{"grpc_code", "CANCELLED"}})
                     .SingleMetricOptional(std::string{kStatus}));
    EXPECT_EQ(get_metric(kAbandoned), 0);

    sample::ugrpc::StreamGreetingRequest request;

    request.set_number(1);
    ASSERT_TRUE(stream1.Write(request));

    request.set_number(42);
    ASSERT_TRUE(stream2.Write(request));

    UASSERT_NO_THROW(stream1 = std::move(stream2));

    ASSERT_TRUE(stream1.WritesDone());

    sample::ugrpc::StreamGreetingResponse response;
    ASSERT_TRUE(stream1.Read(response));
    ASSERT_EQ(response.number(), 42);  // number from stream2.
    ASSERT_FALSE(stream1.Read(response));

    // Stream was read while Read() == false => OK
    EXPECT_EQ(get_metric(kStatus, {{"grpc_code", "OK"}}), 1);

    // Moved stream was cancelled in a destructor.
    EXPECT_FALSE(GetStatistics("grpc.client.total", {{"grpc_code", "CANCELLED"}}).SingleMetricOptional("status"));
    EXPECT_EQ(get_metric(kAbandoned), 1);

    EXPECT_EQ(get_metric("cancelled"), 0);
    EXPECT_EQ(get_metric(kStatus, {{"grpc_code", "UNKNOWN"}}), 0);
}

UTEST_F(GrpcInputStream, InputStreamDestroy) {
    {
        const sample::ugrpc::StreamGreetingRequest request;
        UEXPECT_NO_THROW(const auto stream = GetClient().ReadMany(request));
        // We want to TryCancel and Finish in a destructor without any problem.
    }
    {
        const sample::ugrpc::StreamGreetingRequest request;
        auto stream1 = GetClient().ReadMany(request);
        auto stream2 = GetClient().ReadMany(request);

        sample::ugrpc::StreamGreetingResponse response;

        ASSERT_TRUE(stream1.Read(response));
        ASSERT_EQ(response.number(), 0);

        for (std::size_t i = 0; i < 2; ++i) {
            ASSERT_TRUE(stream2.Read(response));
            ASSERT_EQ(response.number(), i);
        }

        UEXPECT_NO_THROW(stream1 = std::move(stream2));

        ASSERT_TRUE(stream1.Read(response));
        // Expected response from stream2.
        ASSERT_EQ(response.number(), 2);
    }
}

UTEST_F(GrpcInputStream, InputStreamTest) {
    const sample::ugrpc::StreamGreetingRequest request;
    auto stream = GetClient().ReadMany(request);

    sample::ugrpc::StreamGreetingResponse response;
    ASSERT_TRUE(stream.Read(response));
    ASSERT_TRUE(stream.Read(response));
    ASSERT_TRUE(stream.Read(response));
    ASSERT_FALSE(stream.Read(response));

    ASSERT_THAT(ugrpc::client::ReadRemainingAndFinish(stream), testing::Optional(0));
    ASSERT_FALSE(stream.Read(response));
}

UTEST_F(GrpcInputStream, InputStreamReadRemainingNoMessages) {
    const sample::ugrpc::StreamGreetingRequest request;
    auto stream = GetClient().ReadMany(request);

    sample::ugrpc::StreamGreetingResponse response;
    ASSERT_TRUE(stream.Read(response));
    ASSERT_TRUE(stream.Read(response));
    ASSERT_TRUE(stream.Read(response));

    ASSERT_THAT(ugrpc::client::ReadRemainingAndFinish(stream), testing::Optional(0));
    ASSERT_FALSE(stream.Read(response));
}

UTEST_F(GrpcInputStream, InputStreamReadRemainingMultipleMessages) {
    const sample::ugrpc::StreamGreetingRequest request;
    auto stream = GetClient().ReadMany(request);

    sample::ugrpc::StreamGreetingResponse response;
    ASSERT_TRUE(stream.Read(response));

    ASSERT_THAT(ugrpc::client::ReadRemainingAndFinish(stream), testing::Optional(2));
    ASSERT_FALSE(stream.Read(response));
}

UTEST_F(GrpcOutputStream, OutputStreamAlreadyFinish) {
    auto stream = GetClient().WriteMany();

    const sample::ugrpc::StreamGreetingRequest request;
    ASSERT_TRUE(stream.Write(request));
    UEXPECT_NO_THROW(stream.Finish());

    ASSERT_FALSE(stream.Write(sample::ugrpc::StreamGreetingRequest()));
    UEXPECT_THROW_MSG(
        stream.WriteAndCheck(sample::ugrpc::StreamGreetingRequest()),
        ugrpc::client::RpcError,
        "'WriteAndCheck' called on a finished or closed stream"
    );
}

UTEST_F(GrpcOutputStream, OutputStreamDestroy) {
    {
        auto stream = GetClient().WriteMany();
        const sample::ugrpc::StreamGreetingRequest request;
        ASSERT_TRUE(stream.Write(request));
        // We want to TryCancel and Finish in a destructor without any problem.
    }

    auto stream1 = GetClient().WriteMany();
    auto stream2 = GetClient().WriteMany();
    const sample::ugrpc::StreamGreetingRequest request;
    ASSERT_TRUE(stream1.Write(request));
    ASSERT_TRUE(stream2.Write(request));
    ASSERT_TRUE(stream2.Write(request));

    UEXPECT_NO_THROW(stream1 = std::move(stream2));

    const auto response = stream1.Finish();
    // Expected requests count = 2 from stream2.
    ASSERT_EQ(response.number(), 2);
}

USERVER_NAMESPACE_END
