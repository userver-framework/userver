#include <userver/utest/utest.hpp>

#include <gmock/gmock.h>
#include <grpcpp/support/status.h>

#include <tests/deadline_helpers.hpp>
#include <userver/ugrpc/client/exceptions.hpp>
#include <userver/ugrpc/server/exceptions.hpp>
#include <userver/ugrpc/server/middlewares/base.hpp>
#include <userver/ugrpc/server/middlewares/deadline_propagation/middleware.hpp>
#include <userver/ugrpc/tests/service_fixtures.hpp>

#include <userver/engine/sleep.hpp>
#include <userver/server/handlers/exceptions.hpp>
#include <userver/utest/log_capture_fixture.hpp>
#include <userver/utils/flags.hpp>

#include <tests/unit_test_client.usrv.pb.hpp>
#include <tests/unit_test_service.usrv.pb.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

const grpc::Status kUnknownErrorStatus{
    grpc::StatusCode::UNKNOWN,
    "The service method has exited unexpectedly, without providing a status"};

const grpc::Status kUnimplementedStatus{grpc::StatusCode::UNIMPLEMENTED, "This method is unimplemented"};

class MessengerMock final : public sample::ugrpc::UnitTestServiceBase {
public:
    MOCK_METHOD(
        SayHelloResult,
        SayHello,
        (ugrpc::server::CallContext& /*context*/, ::sample::ugrpc::GreetingRequest&& /*request*/),
        (override)
    );
};

struct Flags final {
    bool set_error{true};
};

class MockMiddleware : public ugrpc::server::MiddlewareBase {
public:
    MOCK_METHOD(void, OnCallStart, (ugrpc::server::MiddlewareCallContext & context), (const, override));
    MOCK_METHOD(
        void,
        OnCallFinish,
        (ugrpc::server::MiddlewareCallContext & context, const grpc::Status& status),
        (const, override)
    );
    MOCK_METHOD(
        void,
        PostRecvMessage,
        (ugrpc::server::MiddlewareCallContext & context, google::protobuf::Message&),
        (const, override)
    );
    MOCK_METHOD(
        void,
        PreSendMessage,
        (ugrpc::server::MiddlewareCallContext & context, google::protobuf::Message&),
        (const, override)
    );
};

// NOLINTNEXTLINE(fuchsia-multiple-inheritance)
class MiddlewaresHooksUnaryTest : public ugrpc::tests::ServiceFixtureBase, public testing::WithParamInterface<Flags> {
protected:
    MiddlewaresHooksUnaryTest()
        : m0(std::make_shared<MockMiddleware>()),
          m1(std::make_shared<MockMiddleware>()),
          m2(std::make_shared<MockMiddleware>()) {
        SetServerMiddlewares({m0, m1, m2});
        RegisterService(service_);
        StartServer();
    }

    auto GetClient() { return MakeClient<sample::ugrpc::UnitTestServiceClient>(); }

    MessengerMock& Service() { return service_; }

    void SetSuccessHandler() {
        ON_CALL(Service(), SayHello).WillByDefault([](ugrpc::server::CallContext&, ::sample::ugrpc::GreetingRequest&&) {
            return sample::ugrpc::GreetingResponse{};
        });
    }

    template <typename Handler>
    void SetHandler(Handler&& handler) {
        ON_CALL(Service(), SayHello)
            .WillByDefault([handler = std::forward<Handler>(handler
                            )](ugrpc::server::CallContext&, ::sample::ugrpc::GreetingRequest&&) { return handler(); });
    }

    const MockMiddleware& M0() { return *m0; }
    const MockMiddleware& M1() { return *m1; }
    const MockMiddleware& M2() { return *m2; }

    void SetErrorOrThrowRuntimeError(
        ugrpc::server::MiddlewareCallContext& context,
        grpc::Status status = kUnknownErrorStatus
    ) {
        if (GetParam().set_error) {
            context.SetError(std::move(status));
        } else {
            throw std::runtime_error{"error"};
        }
    }

private:
    MessengerMock service_;
    std::shared_ptr<MockMiddleware> m0;
    std::shared_ptr<MockMiddleware> m1;
    std::shared_ptr<MockMiddleware> m2;
};

}  // namespace

UTEST_P(MiddlewaresHooksUnaryTest, Success) {
    SetSuccessHandler();

    EXPECT_CALL(M1(), OnCallStart).Times(1);
    EXPECT_CALL(M1(), PostRecvMessage).Times(1);
    EXPECT_CALL(M1(), PreSendMessage).Times(1);
    EXPECT_CALL(M1(), OnCallFinish).Times(1);

    EXPECT_CALL(M2(), OnCallStart).Times(1);
    EXPECT_CALL(M2(), PostRecvMessage).Times(1);
    EXPECT_CALL(M2(), PreSendMessage).Times(1);
    EXPECT_CALL(M2(), OnCallFinish).Times(1);

    const auto client = GetClient();
    UEXPECT_NO_THROW(client.SayHello(sample::ugrpc::GreetingRequest()));
}

UTEST_P(MiddlewaresHooksUnaryTest, FailInFirstMiddlewareOnStart) {
    SetSuccessHandler();
    const auto client = GetClient();

    ON_CALL(M1(), OnCallStart).WillByDefault([this](ugrpc::server::MiddlewareCallContext& context) {
        SetErrorOrThrowRuntimeError(context);
    });

    EXPECT_CALL(M1(), OnCallStart).Times(1);
    EXPECT_CALL(M1(), PostRecvMessage).Times(0);
    EXPECT_CALL(M1(), PreSendMessage).Times(0);
    EXPECT_CALL(M1(), OnCallFinish).Times(0);

    // The Pipeline will not reach M2, because there is an error in M1 in OnCallStart.
    EXPECT_CALL(M2(), OnCallStart).Times(0);
    EXPECT_CALL(M2(), PostRecvMessage).Times(0);
    EXPECT_CALL(M2(), PreSendMessage).Times(0);
    EXPECT_CALL(M2(), OnCallFinish).Times(0);

    UEXPECT_THROW(client.SayHello(sample::ugrpc::GreetingRequest()), ugrpc::client::UnknownError);
}

UTEST_P(MiddlewaresHooksUnaryTest, FailInFirstMiddlewareOnPostRecvMessage) {
    SetSuccessHandler();
    const auto client = GetClient();

    ON_CALL(M1(), PostRecvMessage)
        .WillByDefault([this](ugrpc::server::MiddlewareCallContext& context, google::protobuf::Message&) {
            SetErrorOrThrowRuntimeError(context);
        });

    EXPECT_CALL(M1(), OnCallStart).Times(1);
    EXPECT_CALL(M1(), PostRecvMessage).Times(1);
    EXPECT_CALL(M1(), PreSendMessage).Times(0);
    // OnCallStart of M1 is successfully => OnCallFinish must be called.
    EXPECT_CALL(M1(), OnCallFinish).Times(1);

    // The Pipeline will not reach M2, because there is an error in M1 on PostRecvMessage.
    EXPECT_CALL(M2(), OnCallStart).Times(0);
    EXPECT_CALL(M2(), PostRecvMessage).Times(0);
    EXPECT_CALL(M2(), PreSendMessage).Times(0);
    EXPECT_CALL(M2(), OnCallFinish).Times(0);

    UEXPECT_THROW(client.SayHello(sample::ugrpc::GreetingRequest()), ugrpc::client::UnknownError);
}

UTEST_P(MiddlewaresHooksUnaryTest, FailInSecondMiddlewareOnPostRecvMessage) {
    SetSuccessHandler();
    const auto client = GetClient();

    ON_CALL(M2(), PostRecvMessage)
        .WillByDefault([this](ugrpc::server::MiddlewareCallContext& context, google::protobuf::Message&) {
            SetErrorOrThrowRuntimeError(context);
        });

    EXPECT_CALL(M1(), OnCallStart).Times(1);
    EXPECT_CALL(M1(), PostRecvMessage).Times(1);
    EXPECT_CALL(M1(), PreSendMessage).Times(0);
    EXPECT_CALL(M1(), OnCallFinish).Times(1);

    // The Pipeline will not reach M2, because there is an error in M1 on PostRecvMessage.
    EXPECT_CALL(M2(), OnCallStart).Times(1);
    EXPECT_CALL(M2(), PostRecvMessage).Times(1);
    EXPECT_CALL(M2(), PreSendMessage).Times(0);
    // OnCallStart of M2 is successfully => OnCallFinish must be called.
    EXPECT_CALL(M2(), OnCallFinish).Times(1);

    UEXPECT_THROW(client.SayHello(sample::ugrpc::GreetingRequest()), ugrpc::client::UnknownError);
}

UTEST_P(MiddlewaresHooksUnaryTest, FailInSecondMiddlewareOnStart) {
    SetSuccessHandler();
    const auto client = GetClient();

    ON_CALL(M2(), OnCallStart).WillByDefault([this](ugrpc::server::MiddlewareCallContext& context) {
        SetErrorOrThrowRuntimeError(context);
    });

    EXPECT_CALL(M1(), OnCallStart).Times(1);
    EXPECT_CALL(M1(), PostRecvMessage).Times(1);
    EXPECT_CALL(M1(), PreSendMessage).Times(0);
    // OnCallStart of M1 is successfully => OnCallFinish must be called.
    EXPECT_CALL(M1(), OnCallFinish).Times(1);

    EXPECT_CALL(M2(), OnCallStart).Times(1);
    EXPECT_CALL(M2(), PostRecvMessage).Times(0);
    EXPECT_CALL(M2(), PreSendMessage).Times(0);
    EXPECT_CALL(M2(), OnCallFinish).Times(0);

    UEXPECT_THROW(client.SayHello(sample::ugrpc::GreetingRequest()), ugrpc::client::UnknownError);
}

UTEST_P(MiddlewaresHooksUnaryTest, FailInSecondMiddlewarePreSend) {
    SetSuccessHandler();
    const auto client = GetClient();

    ON_CALL(M2(), PreSendMessage)
        .WillByDefault([this](ugrpc::server::MiddlewareCallContext& context, google::protobuf::Message&) {
            SetErrorOrThrowRuntimeError(context);
        });

    EXPECT_CALL(M1(), OnCallStart).Times(1);
    EXPECT_CALL(M1(), PostRecvMessage).Times(1);
    EXPECT_CALL(M1(), PreSendMessage).Times(0);
    // OnCallStart of M1 is successfully => OnCallFinish must be called.
    EXPECT_CALL(M1(), OnCallFinish).Times(1);

    EXPECT_CALL(M2(), OnCallStart).Times(1);
    EXPECT_CALL(M2(), PostRecvMessage).Times(1);
    EXPECT_CALL(M2(), PreSendMessage).Times(1);
    EXPECT_CALL(M2(), OnCallFinish).Times(1);

    UEXPECT_THROW(client.SayHello(sample::ugrpc::GreetingRequest()), ugrpc::client::UnknownError);
}

UTEST_P(MiddlewaresHooksUnaryTest, ApplyTheLastErrorStatus) {
    SetSuccessHandler();
    const auto client = GetClient();

    // The order if OnCallFinish is reversed: M2 -> M1
    ON_CALL(M2(), OnCallFinish)
        .WillByDefault([](ugrpc::server::MiddlewareCallContext& context, const grpc::Status& status) {
            EXPECT_TRUE(status.ok());
            if (GetParam().set_error) {
                context.SetError(grpc::Status{kUnimplementedStatus});
            } else {
                throw ugrpc::server::RpcInterruptedError("call_name", "stage");
            }
        });
    ON_CALL(M1(), OnCallFinish)
        .WillByDefault([](ugrpc::server::MiddlewareCallContext& context, const grpc::Status& status) {
            // That status must be from M2::OnCallFinish
            if (GetParam().set_error) {
                EXPECT_EQ(status.error_code(), kUnimplementedStatus.error_code());
                EXPECT_EQ(status.error_message(), kUnimplementedStatus.error_message());
            } else {
                EXPECT_EQ(status.error_code(), grpc::StatusCode::CANCELLED);
                EXPECT_EQ(status.error_message(), "");
            }
            context.SetError(grpc::Status{kUnknownErrorStatus});
        });

    EXPECT_CALL(M1(), OnCallStart).Times(1);
    EXPECT_CALL(M1(), PostRecvMessage).Times(1);
    EXPECT_CALL(M1(), PreSendMessage).Times(1);
    // OnCallStart of M1 is successfully => OnCallFinish must be called.
    EXPECT_CALL(M1(), OnCallFinish).Times(1);

    EXPECT_CALL(M2(), OnCallStart).Times(1);
    EXPECT_CALL(M2(), PostRecvMessage).Times(1);
    EXPECT_CALL(M2(), PreSendMessage).Times(1);
    // OnCallStart of M2 is successfully => OnCallFinish must be called.
    EXPECT_CALL(M2(), OnCallFinish).Times(1);

    UEXPECT_THROW(client.SayHello(sample::ugrpc::GreetingRequest()), ugrpc::client::UnknownError);
}

UTEST_P(MiddlewaresHooksUnaryTest, ThrowInHandler) {
    SetHandler([] {
        throw server::handlers::Unauthorized{server::handlers::ExternalBody{"fail :("}};
        return sample::ugrpc::GreetingResponse{};
    });
    const auto client = GetClient();

    // The order if OnCallFinish is reversed: M2 -> M1
    ON_CALL(M2(), OnCallFinish)
        .WillByDefault([](ugrpc::server::MiddlewareCallContext& /*context*/, const grpc::Status& status) {
            EXPECT_TRUE(!status.ok());
            EXPECT_EQ(status.error_code(), grpc::StatusCode::UNAUTHENTICATED);
            EXPECT_EQ(status.error_message(), "fail :(");
        });
    ON_CALL(M1(), OnCallFinish)
        .WillByDefault([](ugrpc::server::MiddlewareCallContext& /*context*/, const grpc::Status& status) {
            EXPECT_TRUE(!status.ok());
            EXPECT_EQ(status.error_code(), grpc::StatusCode::UNAUTHENTICATED);
            EXPECT_EQ(status.error_message(), "fail :(");
        });

    EXPECT_CALL(M1(), OnCallStart).Times(1);
    EXPECT_CALL(M1(), PostRecvMessage).Times(1);
    EXPECT_CALL(M1(), PreSendMessage).Times(0);
    // OnCallStart of M1 is successfully => OnCallFinish must be called.
    EXPECT_CALL(M1(), OnCallFinish).Times(1);

    EXPECT_CALL(M2(), OnCallStart).Times(1);
    EXPECT_CALL(M2(), PostRecvMessage).Times(1);
    EXPECT_CALL(M2(), PreSendMessage).Times(0);
    // OnCallStart of M2 is successfully => OnCallFinish must be called.
    EXPECT_CALL(M2(), OnCallFinish).Times(1);

    UEXPECT_THROW(client.SayHello(sample::ugrpc::GreetingRequest()), ugrpc::client::UnauthenticatedError);
}

UTEST_P(MiddlewaresHooksUnaryTest, DeadlinePropagation) {
    SetSuccessHandler();
    const auto client = GetClient();

    ugrpc::server::middlewares::deadline_propagation::Middleware deadline_propagation{};

    ON_CALL(M1(), OnCallStart).WillByDefault([&deadline_propagation](ugrpc::server::MiddlewareCallContext& context) {
        deadline_propagation.OnCallStart(context);
    });

    // The order if OnCallFinish is reversed: M2 -> M1 -> M0
    ON_CALL(M2(), OnCallFinish)
        .WillByDefault([](ugrpc::server::MiddlewareCallContext& /*context*/, const grpc::Status& status) {
            EXPECT_TRUE(status.ok());
            // We want to exceed a deadline for middleware 'grpc-server-deadline-propagation'
            engine::SleepFor(std::chrono::milliseconds{20});
        });
    ON_CALL(M1(), OnCallFinish)
        .WillByDefault(
            [&deadline_propagation](ugrpc::server::MiddlewareCallContext& context, const grpc::Status& status) {
                EXPECT_TRUE(status.ok());
                /// Here the status will be replaced by 'grpc-server-deadline-propagation' middleware
                deadline_propagation.OnCallFinish(context, status);
            }
        );
    ON_CALL(M0(), OnCallFinish)
        .WillByDefault([](ugrpc::server::MiddlewareCallContext& /*context*/, const grpc::Status& status) {
            // Status from 'grpc-server-deadline-propagation' middleware
            EXPECT_TRUE(!status.ok());
            EXPECT_EQ(status.error_code(), grpc::StatusCode::DEADLINE_EXCEEDED);
            const auto& msg = status.error_message();
            EXPECT_EQ("Deadline specified by the client for this RPC was exceeded", msg);
        });

    EXPECT_CALL(M1(), OnCallStart).Times(1);
    EXPECT_CALL(M1(), PostRecvMessage).Times(1);
    EXPECT_CALL(M1(), PreSendMessage).Times(1);
    // OnCallStart of M1 is successfully => OnCallFinish must be called.
    EXPECT_CALL(M1(), OnCallFinish).Times(1);

    EXPECT_CALL(M2(), OnCallStart).Times(1);
    EXPECT_CALL(M2(), PostRecvMessage).Times(1);
    EXPECT_CALL(M2(), PreSendMessage).Times(1);
    // OnCallStart of M2 is successfully => OnCallFinish must be called.
    EXPECT_CALL(M2(), OnCallFinish).Times(1);

    auto context = std::make_unique<grpc::ClientContext>();
    std::chrono::milliseconds deadline_ms{10};
    auto deadline = engine::Deadline::FromDuration(deadline_ms);
    context->set_deadline(deadline);

    UEXPECT_THROW(
        client.SayHello(sample::ugrpc::GreetingRequest(), std::move(context)), ugrpc::client::DeadlineExceededError
    );
}

INSTANTIATE_UTEST_SUITE_P(
    /*no prefix*/,
    MiddlewaresHooksUnaryTest,
    testing::Values(Flags{/*set_error=*/true}, Flags{/*set_error*/ false})
);

USERVER_NAMESPACE_END
