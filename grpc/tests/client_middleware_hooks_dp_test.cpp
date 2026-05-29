#include <userver/utest/utest.hpp>

#include <userver/engine/sleep.hpp>

#include <dynamic_config/variables/USERVER_GRPC_CLIENT_ENABLE_DEADLINE_PROPAGATION.hpp>

#include <userver/ugrpc/client/middlewares/deadline_propagation/middleware.hpp>
#include <userver/ugrpc/tests/service_fixtures.hpp>

#include <tests/client_middleware_base_gmock.hpp>
#include <tests/deadline_helpers.hpp>
#include <tests/unit_test_client.usrv.pb.hpp>
#include <tests/unit_test_service.usrv.pb.hpp>
#include <tests/unit_test_service_gmock.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

class ClientMiddlewaresHooksDPTest : public ugrpc::tests::ServiceFixtureBase {
    using MiddlewareMock = ::testing::NiceMock<tests::client::ClientMiddlewareBaseMock>;
    using ClientType = sample::ugrpc::UnitTestServiceClient;
    using ServiceType = ::testing::NiceMock<tests::UnitTestServiceGmock>;

public:
    using CallContext = ugrpc::server::CallContext;

    using Request = sample::ugrpc::GreetingRequest;
    using Response = sample::ugrpc::GreetingResponse;

    using StreamRequest = sample::ugrpc::StreamGreetingRequest;
    using StreamResponse = sample::ugrpc::StreamGreetingResponse;

    using UnaryResult = tests::UnitTestServiceGmock::SayHelloResult;
    using ServerStreamingResult = tests::UnitTestServiceGmock::ReadManyResult;
    using ClientStreamingResult = tests::UnitTestServiceGmock::WriteManyResult;
    using BidirectionalStreamingResult = tests::UnitTestServiceGmock::ChatResult;

    using Writer = tests::UnitTestServiceGmock::ReadManyWriter;
    using Reader = tests::UnitTestServiceGmock::WriteManyReader;
    using ReaderWriter = tests::UnitTestServiceGmock::ChatReaderWriter;

    ClientMiddlewaresHooksDPTest()
        : middleware_mock_{std::make_shared<MiddlewareMock>()}
    {
        SetClientMiddlewares(
            {std::make_shared<ugrpc::client::middlewares::deadline_propagation::Middleware>(), middleware_mock_}
        );
        RegisterService(service_);
        StartServer();
        client_.emplace(MakeClient<ClientType>());
    }
    ~ClientMiddlewaresHooksDPTest() override {
        client_.reset();
        StopServer();
    }
    ServiceType& Service() { return service_; }
    ClientType& Client() { return client_.value(); }
    const MiddlewareMock& Middleware() const { return *middleware_mock_; }

private:
    ServiceType service_{};
    std::optional<ClientType> client_{};
    std::shared_ptr<const MiddlewareMock> middleware_mock_{};
};

auto ExpectSpecialCase(ugrpc::client::SpecialCaseCompletionType expected_type) {
    return [expected_type](const ugrpc::client::MiddlewareCallContext&, const ugrpc::client::CompletionStatus& result) {
        EXPECT_FALSE(result.has_value());
        EXPECT_EQ(result.error(), expected_type);
    };
}

}  // namespace

UTEST_F(ClientMiddlewaresHooksDPTest, DeadlinePropagationUnary) {
    ExtendDynamicConfig({{::dynamic_config::USERVER_GRPC_CLIENT_ENABLE_DEADLINE_PROPAGATION, true}});
    ON_CALL(Service(), SayHello).WillByDefault([](CallContext&, Request&&) -> UnaryResult {
        engine::InterruptibleSleepFor(utest::kMaxTestWaitTime);
        return Response{};
    });

    EXPECT_CALL(Middleware(), PreStartCall).Times(1);
    EXPECT_CALL(Middleware(), PreSendMessage).Times(1);
    EXPECT_CALL(Middleware(), PostRecvMessage).Times(0);
    EXPECT_CALL(Middleware(), PostFinish)
        .Times(1)
        .WillOnce(ExpectSpecialCase(ugrpc::client::SpecialCaseCompletionType::kTimeoutDeadlinePropagated));

    tests::InitTaskInheritedDeadline(engine::Deadline::FromDuration(tests::kShortTimeout));

    ugrpc::client::CallOptions call_options;
    call_options.SetTimeout(tests::kLongTimeout);
    sample::ugrpc::GreetingRequest request;
    request.set_name("userver");
    UEXPECT_THROW_MSG(
        Client().SayHello(request, std::move(call_options)),
        ugrpc::client::RpcCancelledError,
        "Deadline Propagation"
    );
}

UTEST_F(ClientMiddlewaresHooksDPTest, DeadlinePropagationServerStreaming) {
    ExtendDynamicConfig({{::dynamic_config::USERVER_GRPC_CLIENT_ENABLE_DEADLINE_PROPAGATION, true}});

    ON_CALL(Service(), ReadMany)
        .WillByDefault([](CallContext&, StreamRequest&&, Writer& writer) -> ServerStreamingResult {
            StreamResponse response;
            response.set_name("first");
            writer.Write(std::move(response));

            engine::InterruptibleSleepFor(utest::kMaxTestWaitTime);
            return grpc::Status::OK;
        });

    EXPECT_CALL(Middleware(), PreStartCall).Times(1);
    EXPECT_CALL(Middleware(), PreSendMessage).Times(1);
    EXPECT_CALL(Middleware(), PostRecvMessage).Times(1);
    EXPECT_CALL(Middleware(), PostFinish)
        .Times(1)
        .WillOnce(ExpectSpecialCase(ugrpc::client::SpecialCaseCompletionType::kTimeoutDeadlinePropagated));

    tests::InitTaskInheritedDeadline(engine::Deadline::FromDuration(tests::kShortTimeout));

    ugrpc::client::CallOptions call_options;
    call_options.SetTimeout(tests::kLongTimeout);

    StreamRequest request;
    request.set_name("userver");
    request.set_number(1);
    StreamResponse response;

    auto stream = Client().ReadMany(request, std::move(call_options));
    EXPECT_TRUE(stream.Read(response));
    UEXPECT_THROW_MSG(
        [[maybe_unused]] auto ok = stream.Read(response),
        ugrpc::client::RpcCancelledError,
        "Deadline Propagation"
    );
}

UTEST_F(ClientMiddlewaresHooksDPTest, DeadlinePropagationClientStreaming) {
    ExtendDynamicConfig({{::dynamic_config::USERVER_GRPC_CLIENT_ENABLE_DEADLINE_PROPAGATION, true}});

    engine::SingleUseEvent server_started;
    ON_CALL(Service(), WriteMany).WillByDefault([&server_started](CallContext&, Reader&) -> ClientStreamingResult {
        server_started.Send();
        engine::InterruptibleSleepFor(utest::kMaxTestWaitTime);
        return StreamResponse{};
    });

    EXPECT_CALL(Middleware(), PreStartCall).Times(1);
    EXPECT_CALL(Middleware(), PreSendMessage).Times(0);
    EXPECT_CALL(Middleware(), PostRecvMessage).Times(0);
    EXPECT_CALL(Middleware(), PostFinish)
        .Times(1)
        .WillOnce(ExpectSpecialCase(ugrpc::client::SpecialCaseCompletionType::kTimeoutDeadlinePropagated));

    tests::InitTaskInheritedDeadline(engine::Deadline::FromDuration(tests::kShortTimeout));

    ugrpc::client::CallOptions call_options;
    call_options.SetTimeout(tests::kLongTimeout);

    auto stream = Client().WriteMany(std::move(call_options));
    server_started.Wait();
    UEXPECT_THROW_MSG(
        [[maybe_unused]] auto response = stream.Finish(),
        ugrpc::client::RpcCancelledError,
        "Deadline Propagation"
    );
}

UTEST_F(ClientMiddlewaresHooksDPTest, DeadlinePropagationBidirectionalStreaming) {
    ExtendDynamicConfig({{::dynamic_config::USERVER_GRPC_CLIENT_ENABLE_DEADLINE_PROPAGATION, true}});
    ON_CALL(Service(), Chat).WillByDefault([](CallContext&, ReaderWriter& stream) -> BidirectionalStreamingResult {
        StreamRequest request;
        EXPECT_TRUE(stream.Read(request));

        StreamResponse response;
        response.set_name("Hello " + request.name());
        stream.Write(std::move(response));

        engine::InterruptibleSleepFor(utest::kMaxTestWaitTime);
        return grpc::Status::OK;
    });

    EXPECT_CALL(Middleware(), PreStartCall).Times(1);
    EXPECT_CALL(Middleware(), PreSendMessage).Times(1);
    EXPECT_CALL(Middleware(), PostRecvMessage).Times(1);
    EXPECT_CALL(Middleware(), PostFinish)
        .Times(1)
        .WillOnce(ExpectSpecialCase(ugrpc::client::SpecialCaseCompletionType::kTimeoutDeadlinePropagated));

    tests::InitTaskInheritedDeadline(engine::Deadline::FromDuration(tests::kShortTimeout));

    ugrpc::client::CallOptions call_options;
    call_options.SetTimeout(tests::kLongTimeout);

    StreamRequest request;
    request.set_name("userver");
    StreamResponse response;

    auto stream = Client().Chat(std::move(call_options));
    // NOLINTNEXTLINE(clang-analyzer-optin.cplusplus.UninitializedObject)
    stream.WriteAndCheck(request);
    EXPECT_TRUE(stream.Read(response));
    UEXPECT_THROW_MSG(
        [[maybe_unused]] auto ok = stream.Read(response),
        ugrpc::client::RpcCancelledError,
        "Deadline Propagation"
    );
}

USERVER_NAMESPACE_END
