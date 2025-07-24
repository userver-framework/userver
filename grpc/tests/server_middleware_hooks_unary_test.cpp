#include <userver/utest/utest.hpp>

#include <gmock/gmock.h>
#include <grpcpp/support/status.h>

#include <tests/deadline_helpers.hpp>
#include <tests/middlewares_fixture.hpp>
#include <tests/server_middleware_base_gmock.hpp>
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

class UnitTestServiceMock : public sample::ugrpc::UnitTestServiceBase {
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

// NOLINTNEXTLINE(fuchsia-multiple-inheritance)
class ServerMiddlewareHooksUnaryTest : public tests::MiddlewaresFixture<
                                           tests::server::ServerMiddlewareBaseMock,
                                           ::testing::NiceMock<UnitTestServiceMock>,
                                           sample::ugrpc::UnitTestServiceClient,
                                           /*N=*/3>,
                                       public testing::WithParamInterface<Flags> {
protected:
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
};

}  // namespace

UTEST_P(ServerMiddlewareHooksUnaryTest, Success) {
    SetSuccessHandler();

    EXPECT_CALL(Middleware(1), OnCallStart).Times(1);
    EXPECT_CALL(Middleware(1), PostRecvMessage).Times(1);
    EXPECT_CALL(Middleware(1), PreSendMessage).Times(1);
    EXPECT_CALL(Middleware(1), OnCallFinish).Times(1);

    EXPECT_CALL(Middleware(2), OnCallStart).Times(1);
    EXPECT_CALL(Middleware(2), PostRecvMessage).Times(1);
    EXPECT_CALL(Middleware(2), PreSendMessage).Times(1);
    EXPECT_CALL(Middleware(2), OnCallFinish).Times(1);

    UEXPECT_NO_THROW(Client().SayHello(sample::ugrpc::GreetingRequest()));
}

UTEST_P(ServerMiddlewareHooksUnaryTest, FailInFirstMiddlewareOnStart) {
    SetSuccessHandler();

    ON_CALL(Middleware(1), OnCallStart).WillByDefault([this](ugrpc::server::MiddlewareCallContext& context) {
        SetErrorOrThrowRuntimeError(context);
    });

    EXPECT_CALL(Middleware(1), OnCallStart).Times(1);
    EXPECT_CALL(Middleware(1), PostRecvMessage).Times(0);
    EXPECT_CALL(Middleware(1), PreSendMessage).Times(0);
    EXPECT_CALL(Middleware(1), OnCallFinish).Times(0);

    // The Pipeline will not reach M2, because there is an error in M1 in OnCallStart.
    EXPECT_CALL(Middleware(2), OnCallStart).Times(0);
    EXPECT_CALL(Middleware(2), PostRecvMessage).Times(0);
    EXPECT_CALL(Middleware(2), PreSendMessage).Times(0);
    EXPECT_CALL(Middleware(2), OnCallFinish).Times(0);

    UEXPECT_THROW(Client().SayHello(sample::ugrpc::GreetingRequest()), ugrpc::client::UnknownError);
}

UTEST_P(ServerMiddlewareHooksUnaryTest, FailInFirstMiddlewareOnPostRecvMessage) {
    SetSuccessHandler();

    ON_CALL(Middleware(1), PostRecvMessage)
        .WillByDefault([this](ugrpc::server::MiddlewareCallContext& context, google::protobuf::Message&) {
            SetErrorOrThrowRuntimeError(context);
        });

    EXPECT_CALL(Middleware(1), OnCallStart).Times(1);
    EXPECT_CALL(Middleware(1), PostRecvMessage).Times(1);
    EXPECT_CALL(Middleware(1), PreSendMessage).Times(0);
    // OnCallStart of M1 is successfully => OnCallFinish must be called.
    EXPECT_CALL(Middleware(1), OnCallFinish).Times(1);

    // The Pipeline will not reach M2, because there is an error in M1 on PostRecvMessage.
    EXPECT_CALL(Middleware(2), OnCallStart).Times(0);
    EXPECT_CALL(Middleware(2), PostRecvMessage).Times(0);
    EXPECT_CALL(Middleware(2), PreSendMessage).Times(0);
    EXPECT_CALL(Middleware(2), OnCallFinish).Times(0);

    UEXPECT_THROW(Client().SayHello(sample::ugrpc::GreetingRequest()), ugrpc::client::UnknownError);
}

UTEST_P(ServerMiddlewareHooksUnaryTest, FailInSecondMiddlewareOnPostRecvMessage) {
    SetSuccessHandler();

    ON_CALL(Middleware(2), PostRecvMessage)
        .WillByDefault([this](ugrpc::server::MiddlewareCallContext& context, google::protobuf::Message&) {
            SetErrorOrThrowRuntimeError(context);
        });

    EXPECT_CALL(Middleware(1), OnCallStart).Times(1);
    EXPECT_CALL(Middleware(1), PostRecvMessage).Times(1);
    EXPECT_CALL(Middleware(1), PreSendMessage).Times(0);
    EXPECT_CALL(Middleware(1), OnCallFinish).Times(1);

    // The Pipeline will not reach M2, because there is an error in M1 on PostRecvMessage.
    EXPECT_CALL(Middleware(2), OnCallStart).Times(1);
    EXPECT_CALL(Middleware(2), PostRecvMessage).Times(1);
    EXPECT_CALL(Middleware(2), PreSendMessage).Times(0);
    // OnCallStart of M2 is successfully => OnCallFinish must be called.
    EXPECT_CALL(Middleware(2), OnCallFinish).Times(1);

    UEXPECT_THROW(Client().SayHello(sample::ugrpc::GreetingRequest()), ugrpc::client::UnknownError);
}

UTEST_P(ServerMiddlewareHooksUnaryTest, FailInSecondMiddlewareOnStart) {
    SetSuccessHandler();

    ON_CALL(Middleware(2), OnCallStart).WillByDefault([this](ugrpc::server::MiddlewareCallContext& context) {
        SetErrorOrThrowRuntimeError(context);
    });

    EXPECT_CALL(Middleware(1), OnCallStart).Times(1);
    EXPECT_CALL(Middleware(1), PostRecvMessage).Times(1);
    EXPECT_CALL(Middleware(1), PreSendMessage).Times(0);
    // OnCallStart of M1 is successfully => OnCallFinish must be called.
    EXPECT_CALL(Middleware(1), OnCallFinish).Times(1);

    EXPECT_CALL(Middleware(2), OnCallStart).Times(1);
    EXPECT_CALL(Middleware(2), PostRecvMessage).Times(0);
    EXPECT_CALL(Middleware(2), PreSendMessage).Times(0);
    EXPECT_CALL(Middleware(2), OnCallFinish).Times(0);

    UEXPECT_THROW(Client().SayHello(sample::ugrpc::GreetingRequest()), ugrpc::client::UnknownError);
}

UTEST_P(ServerMiddlewareHooksUnaryTest, FailInSecondMiddlewarePreSend) {
    SetSuccessHandler();

    ON_CALL(Middleware(2), PreSendMessage)
        .WillByDefault([this](ugrpc::server::MiddlewareCallContext& context, google::protobuf::Message&) {
            SetErrorOrThrowRuntimeError(context);
        });

    EXPECT_CALL(Middleware(1), OnCallStart).Times(1);
    EXPECT_CALL(Middleware(1), PostRecvMessage).Times(1);
    EXPECT_CALL(Middleware(1), PreSendMessage).Times(0);
    // OnCallStart of M1 is successfully => OnCallFinish must be called.
    EXPECT_CALL(Middleware(1), OnCallFinish).Times(1);

    EXPECT_CALL(Middleware(2), OnCallStart).Times(1);
    EXPECT_CALL(Middleware(2), PostRecvMessage).Times(1);
    EXPECT_CALL(Middleware(2), PreSendMessage).Times(1);
    EXPECT_CALL(Middleware(2), OnCallFinish).Times(1);

    UEXPECT_THROW(Client().SayHello(sample::ugrpc::GreetingRequest()), ugrpc::client::UnknownError);
}

UTEST_P(ServerMiddlewareHooksUnaryTest, ApplyTheLastErrorStatus) {
    SetSuccessHandler();

    // The order if OnCallFinish is reversed: M2 -> M1
    ON_CALL(Middleware(2), OnCallFinish)
        .WillByDefault([](ugrpc::server::MiddlewareCallContext& context, const grpc::Status& status) {
            EXPECT_TRUE(status.ok());
            if (GetParam().set_error) {
                context.SetError(grpc::Status{kUnimplementedStatus});
            } else {
                throw std::runtime_error("Something bad happened in OnCallFinish");
            }
        });
    ON_CALL(Middleware(1), OnCallFinish)
        .WillByDefault([](ugrpc::server::MiddlewareCallContext& context, const grpc::Status& status) {
            // That status must be from M2::OnCallFinish
            if (GetParam().set_error) {
                EXPECT_EQ(status.error_code(), kUnimplementedStatus.error_code());
                EXPECT_EQ(status.error_message(), kUnimplementedStatus.error_message());
            } else {
                EXPECT_EQ(status.error_code(), grpc::StatusCode::UNKNOWN);
                EXPECT_EQ(
                    status.error_message(), "The service method has exited unexpectedly, without providing a status"
                );
            }
            context.SetError(grpc::Status{kUnknownErrorStatus});
        });

    EXPECT_CALL(Middleware(1), OnCallStart).Times(1);
    EXPECT_CALL(Middleware(1), PostRecvMessage).Times(1);
    EXPECT_CALL(Middleware(1), PreSendMessage).Times(1);
    // OnCallStart of M1 is successfully => OnCallFinish must be called.
    EXPECT_CALL(Middleware(1), OnCallFinish).Times(1);

    EXPECT_CALL(Middleware(2), OnCallStart).Times(1);
    EXPECT_CALL(Middleware(2), PostRecvMessage).Times(1);
    EXPECT_CALL(Middleware(2), PreSendMessage).Times(1);
    // OnCallStart of M2 is successfully => OnCallFinish must be called.
    EXPECT_CALL(Middleware(2), OnCallFinish).Times(1);

    UEXPECT_THROW(Client().SayHello(sample::ugrpc::GreetingRequest()), ugrpc::client::UnknownError);
}

UTEST_P(ServerMiddlewareHooksUnaryTest, ThrowInHandler) {
    SetHandler([] {
        throw server::handlers::Unauthorized{server::handlers::ExternalBody{"fail :("}};
        return sample::ugrpc::GreetingResponse{};
    });

    // The order if OnCallFinish is reversed: M2 -> M1
    ON_CALL(Middleware(2), OnCallFinish)
        .WillByDefault([](ugrpc::server::MiddlewareCallContext& /*context*/, const grpc::Status& status) {
            EXPECT_TRUE(!status.ok());
            EXPECT_EQ(status.error_code(), grpc::StatusCode::UNAUTHENTICATED);
            EXPECT_EQ(status.error_message(), "fail :(");
        });
    ON_CALL(Middleware(1), OnCallFinish)
        .WillByDefault([](ugrpc::server::MiddlewareCallContext& /*context*/, const grpc::Status& status) {
            EXPECT_TRUE(!status.ok());
            EXPECT_EQ(status.error_code(), grpc::StatusCode::UNAUTHENTICATED);
            EXPECT_EQ(status.error_message(), "fail :(");
        });

    EXPECT_CALL(Middleware(1), OnCallStart).Times(1);
    EXPECT_CALL(Middleware(1), PostRecvMessage).Times(1);
    EXPECT_CALL(Middleware(1), PreSendMessage).Times(0);
    // OnCallStart of M1 is successfully => OnCallFinish must be called.
    EXPECT_CALL(Middleware(1), OnCallFinish).Times(1);

    EXPECT_CALL(Middleware(2), OnCallStart).Times(1);
    EXPECT_CALL(Middleware(2), PostRecvMessage).Times(1);
    EXPECT_CALL(Middleware(2), PreSendMessage).Times(0);
    // OnCallStart of M2 is successfully => OnCallFinish must be called.
    EXPECT_CALL(Middleware(2), OnCallFinish).Times(1);

    UEXPECT_THROW(Client().SayHello(sample::ugrpc::GreetingRequest()), ugrpc::client::UnauthenticatedError);
}

UTEST_P(ServerMiddlewareHooksUnaryTest, DeadlinePropagation) {
    SetSuccessHandler();

    ON_CALL(Middleware(1), OnCallStart).WillByDefault([](ugrpc::server::MiddlewareCallContext& context) {
        const ugrpc::server::middlewares::deadline_propagation::Middleware deadline_propagation{};
        deadline_propagation.OnCallStart(context);
    });

    // The order if OnCallFinish is reversed: M2 -> M1 -> M0
    ON_CALL(Middleware(2), OnCallFinish)
        .WillByDefault([](ugrpc::server::MiddlewareCallContext& /*context*/, const grpc::Status& status) {
            EXPECT_TRUE(status.ok());
            // We want to exceed a deadline for middleware 'grpc-server-deadline-propagation'
            engine::SleepFor(std::chrono::milliseconds{200});
        });
    ON_CALL(Middleware(1), OnCallFinish)
        .WillByDefault([](ugrpc::server::MiddlewareCallContext& context, const grpc::Status& status) {
            EXPECT_TRUE(status.ok());
            /// Here the status will be replaced by 'grpc-server-deadline-propagation' middleware
            const ugrpc::server::middlewares::deadline_propagation::Middleware deadline_propagation{};
            deadline_propagation.OnCallFinish(context, status);
        });
    ON_CALL(Middleware(0), OnCallFinish)
        .WillByDefault([](ugrpc::server::MiddlewareCallContext& /*context*/, const grpc::Status& status) {
            // Status from 'grpc-server-deadline-propagation' middleware
            EXPECT_TRUE(!status.ok());
            EXPECT_EQ(status.error_code(), grpc::StatusCode::DEADLINE_EXCEEDED);
            const auto& msg = status.error_message();
            EXPECT_EQ("Deadline specified by the client for this RPC was exceeded", msg);
        });

    EXPECT_CALL(Middleware(1), OnCallStart).Times(1);
    EXPECT_CALL(Middleware(1), PostRecvMessage).Times(1);
    EXPECT_CALL(Middleware(1), PreSendMessage).Times(1);
    // OnCallStart of M1 is successfully => OnCallFinish must be called.
    EXPECT_CALL(Middleware(1), OnCallFinish).Times(1);

    EXPECT_CALL(Middleware(2), OnCallStart).Times(1);
    EXPECT_CALL(Middleware(2), PostRecvMessage).Times(1);
    EXPECT_CALL(Middleware(2), PreSendMessage).Times(1);
    // OnCallStart of M2 is successfully => OnCallFinish must be called.
    EXPECT_CALL(Middleware(2), OnCallFinish).Times(1);

    ugrpc::client::CallOptions call_options;
    const std::chrono::milliseconds timeout_ms{100};
    call_options.SetTimeout(timeout_ms);

    UEXPECT_THROW(
        Client().SayHello(sample::ugrpc::GreetingRequest(), std::move(call_options)),
        ugrpc::client::DeadlineExceededError
    );
}

INSTANTIATE_UTEST_SUITE_P(
    /*no prefix*/,
    ServerMiddlewareHooksUnaryTest,
    testing::Values(Flags{/*set_error=*/true}, Flags{/*set_error*/ false})
);

USERVER_NAMESPACE_END
