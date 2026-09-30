#include <userver/utest/utest.hpp>

#include <cstddef>
#include <optional>
#include <string_view>

#include <gmock/gmock.h>
#include <google/protobuf/message.h>
#include <grpcpp/support/status.h>

#include <tests/deadline_helpers.hpp>
#include <tests/middlewares_fixture.hpp>
#include <tests/server_middleware_base_gmock.hpp>
#include <userver/ugrpc/client/exceptions.hpp>
#include <userver/ugrpc/server/exceptions.hpp>
#include <userver/ugrpc/server/generic_service_base.hpp>
#include <userver/ugrpc/server/middlewares/base.hpp>
#include <userver/ugrpc/server/middlewares/deadline_propagation/middleware.hpp>
#include <userver/ugrpc/tests/service_fixtures.hpp>

#include <userver/engine/sleep.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/logging/level.hpp>
#include <userver/server/handlers/exceptions.hpp>
#include <userver/utest/log_capture_fixture.hpp>
#include <userver/utils/flags.hpp>

#include <tests/unit_test_client.usrv.pb.hpp>
#include <tests/unit_test_service.usrv.pb.hpp>
#include <tests/unit_test_service_gmock.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

const grpc::Status kUnknownErrorStatus{
    grpc::StatusCode::UNKNOWN,
    "The service method has exited unexpectedly, without providing a status"
};

const grpc::Status kUnimplementedStatus{grpc::StatusCode::UNIMPLEMENTED, "This method is unimplemented"};

struct Flags final {
    bool set_error{true};
};

// NOLINTNEXTLINE(fuchsia-multiple-inheritance)
class ServerMiddlewareHooksUnaryTest
    : public tests::MiddlewaresFixture<
          tests::server::ServerMiddlewareBaseMock,
          ::testing::NiceMock<tests::UnitTestServiceGmock>,
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
    ON_CALL(Middleware(2), PreSendStatus)
        .WillByDefault([](ugrpc::server::MiddlewareCallContext& context, grpc::Status& status) {
            EXPECT_TRUE(status.ok());
            if (GetParam().set_error) {
                context.SetError(grpc::Status{kUnimplementedStatus});
            } else {
                throw std::runtime_error("Something bad happened in OnCallFinish");
            }
        });
    ON_CALL(Middleware(1), PreSendStatus)
        .WillByDefault([](ugrpc::server::MiddlewareCallContext& context, grpc::Status& status) {
            // That status must be from M2::OnCallFinish
            if (GetParam().set_error) {
                EXPECT_EQ(status.error_code(), kUnimplementedStatus.error_code());
                EXPECT_EQ(status.error_message(), kUnimplementedStatus.error_message());
            } else {
                EXPECT_EQ(status.error_code(), grpc::StatusCode::UNKNOWN);
                EXPECT_EQ(
                    status.error_message(),
                    "The service method has exited unexpectedly, without providing a status"
                );
            }
            context.SetError(grpc::Status{kUnknownErrorStatus});
        });

    EXPECT_CALL(Middleware(1), OnCallStart).Times(1);
    EXPECT_CALL(Middleware(1), PostRecvMessage).Times(1);
    EXPECT_CALL(Middleware(1), PreSendMessage).Times(0);
    EXPECT_CALL(Middleware(1), PreSendStatus).Times(1);
    // OnCallStart of M1 is successfully => OnCallFinish must be called.
    EXPECT_CALL(Middleware(1), OnCallFinish).Times(1);

    EXPECT_CALL(Middleware(2), OnCallStart).Times(1);
    EXPECT_CALL(Middleware(2), PostRecvMessage).Times(1);
    EXPECT_CALL(Middleware(2), PreSendMessage).Times(1);
    EXPECT_CALL(Middleware(2), PreSendStatus).Times(1);
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
    ON_CALL(Middleware(2), PreSendStatus)
        .WillByDefault([](ugrpc::server::MiddlewareCallContext& /*context*/, grpc::Status& status) {
            EXPECT_TRUE(!status.ok());
            EXPECT_EQ(status.error_code(), grpc::StatusCode::UNAUTHENTICATED);
            EXPECT_EQ(status.error_message(), "fail :(");
        });
    ON_CALL(Middleware(1), PreSendStatus)
        .WillByDefault([](ugrpc::server::MiddlewareCallContext& /*context*/, grpc::Status& status) {
            EXPECT_TRUE(!status.ok());
            EXPECT_EQ(status.error_code(), grpc::StatusCode::UNAUTHENTICATED);
            EXPECT_EQ(status.error_message(), "fail :(");
        });

    EXPECT_CALL(Middleware(1), OnCallStart).Times(1);
    EXPECT_CALL(Middleware(1), PostRecvMessage).Times(1);
    EXPECT_CALL(Middleware(1), PreSendMessage).Times(0);
    EXPECT_CALL(Middleware(1), PreSendStatus).Times(1);
    // OnCallStart of M1 is successfully => OnCallFinish must be called.
    EXPECT_CALL(Middleware(1), OnCallFinish).Times(1);

    EXPECT_CALL(Middleware(2), OnCallStart).Times(1);
    EXPECT_CALL(Middleware(2), PostRecvMessage).Times(1);
    EXPECT_CALL(Middleware(2), PreSendMessage).Times(0);
    EXPECT_CALL(Middleware(2), PreSendStatus).Times(1);
    // OnCallStart of M2 is successfully => OnCallFinish must be called.
    EXPECT_CALL(Middleware(2), OnCallFinish).Times(1);

    UEXPECT_THROW(Client().SayHello(sample::ugrpc::GreetingRequest()), ugrpc::client::UnauthenticatedError);
}

// The test verifies that in case of deadline propagation, the client receives
// DEADLINE_EXCEEDED and the deadline propagation server middleware forces the
// status to DEADLINE_EXCEEDED.
UTEST_P(ServerMiddlewareHooksUnaryTest, DeadlinePropagation) {
    SetSuccessHandler();

    std::atomic<int> m0_pre_send_status{0};

    std::atomic<int> m1_on_call_start{0};
    std::atomic<int> m1_pre_send_status{0};

    std::atomic<int> m2_pre_send_status{0};

    engine::SingleUseEvent all_middlewares_finished{};

    ON_CALL(Middleware(1), OnCallStart)
        .WillByDefault([&m1_on_call_start](ugrpc::server::MiddlewareCallContext& context) {
            m1_on_call_start.fetch_add(1);
            const ugrpc::server::middlewares::deadline_propagation::Middleware deadline_propagation{
                ugrpc::server::middlewares::deadline_propagation::Settings{}
            };
            deadline_propagation.OnCallStart(context);
        });

    // The order if OnCallFinish is reversed: M2 -> M1 -> M0
    ON_CALL(Middleware(2), PreSendStatus)
        .WillByDefault([&m2_pre_send_status](ugrpc::server::MiddlewareCallContext& /*context*/, grpc::Status& status) {
            m2_pre_send_status.fetch_add(1);
            EXPECT_TRUE(status.ok());
            // We want to exceed a deadline for middleware 'grpc-server-deadline-propagation'
            engine::SleepFor(std::chrono::milliseconds{600});
        });
    ON_CALL(Middleware(1), PreSendStatus)
        .WillByDefault([&m1_pre_send_status](ugrpc::server::MiddlewareCallContext& context, grpc::Status& status) {
            m1_pre_send_status.fetch_add(1);
            EXPECT_TRUE(status.ok());
            /// Here the status will be replaced by 'grpc-server-deadline-propagation' middleware
            const ugrpc::server::middlewares::deadline_propagation::Middleware deadline_propagation{
                ugrpc::server::middlewares::deadline_propagation::Settings{}
            };
            deadline_propagation.PreSendStatus(context, status);
        });
    ON_CALL(Middleware(0), PreSendStatus)
        .WillByDefault([&m0_pre_send_status](ugrpc::server::MiddlewareCallContext& /*context*/, grpc::Status& status) {
            m0_pre_send_status.fetch_add(1);
            // Status from 'grpc-server-deadline-propagation' middleware
            EXPECT_TRUE(!status.ok());
            EXPECT_EQ(status.error_code(), grpc::StatusCode::DEADLINE_EXCEEDED);
        });
    ON_CALL(Middleware(0), OnCallFinish)
        .WillByDefault([&all_middlewares_finished](
                           ugrpc::server::MiddlewareCallContext& /*context*/,
                           const std::optional<grpc::Status>& /*status*/
                       ) { all_middlewares_finished.Send(); });

    ugrpc::client::CallOptions call_options;
    const std::chrono::milliseconds timeout_ms{500};
    call_options.SetTimeout(timeout_ms);

    UEXPECT_THROW(
        Client().SayHello(sample::ugrpc::GreetingRequest(), std::move(call_options)),
        ugrpc::client::DeadlineExceededError
    );

    if (m1_on_call_start.load() == 0) {
        GTEST_SKIP()
            << "RPC was cancelled at the transport layer before reaching the "
               "server, so no middleware hooks ran. Nothing to verify.";
    }

    all_middlewares_finished.WaitNonCancellable();

    EXPECT_EQ(m1_on_call_start.load(), 1);

    if (m2_pre_send_status.load() != 0) {
        EXPECT_EQ(m2_pre_send_status.load(), 1);

        EXPECT_EQ(m1_pre_send_status.load(), 1);

        EXPECT_EQ(m0_pre_send_status.load(), 1);
    }
}

INSTANTIATE_UTEST_SUITE_P(
    /*no prefix*/,
    ServerMiddlewareHooksUnaryTest,
    testing::Values(Flags{.set_error = true}, Flags{.set_error = false})
);

namespace {

constexpr std::size_t kCancellationMiddlewareCount = 2;
constexpr std::size_t kFirstMiddleware = 0;
constexpr std::size_t kLastMiddleware = kCancellationMiddlewareCount - 1;
constexpr std::string_view kUnexpectedExceptionLog = "Uncaught unexpected exception";
constexpr std::string_view kUnexpectedOnCallFinishLog = "Uncaught unexpected exception in OnCallFinish";

void CancelCurrentTask() {
    engine::current_task::RequestCancel();
    engine::current_task::CancellationPoint();
}

struct UnexpectedHandlerException final {};

class ServerMiddlewareUnexpectedExceptionsTest
    : public utest::LogCaptureFixture<tests::MiddlewaresFixture<
          tests::server::ServerMiddlewareBaseMock,
          testing::NiceMock<tests::UnitTestServiceGmock>,
          sample::ugrpc::UnitTestServiceClient,
          kCancellationMiddlewareCount>> {
protected:
    ServerMiddlewareUnexpectedExceptionsTest() {
        ON_CALL(Service(), SayHello).WillByDefault([](ugrpc::server::CallContext&, sample::ugrpc::GreetingRequest&&) {
            return sample::ugrpc::GreetingResponse{};
        });
    }

    void ExpectNoPreFinishHooks(std::size_t index) {
        EXPECT_CALL(Middleware(index), PreSendMessage).Times(0);
        EXPECT_CALL(Middleware(index), PreSendStatus).Times(0);
    }

    void ExpectCancellationCleanup() {
        for (std::size_t index = 0; index < kCancellationMiddlewareCount; ++index) {
            EXPECT_CALL(Middleware(index), OnCallFinish(testing::_, testing::Eq(std::nullopt))).Times(1);
        }
    }

    void ExpectPreSendStatus(std::size_t index, grpc::StatusCode code) {
        EXPECT_CALL(Middleware(index), PreSendStatus)
            .WillOnce([code](ugrpc::server::MiddlewareCallContext&, grpc::Status& status) {
                EXPECT_EQ(status.error_code(), code);
            });
    }

    void ExpectFinishedWithStatus(grpc::StatusCode code) {
        for (std::size_t index = 0; index < kCancellationMiddlewareCount; ++index) {
            EXPECT_CALL(Middleware(index), OnCallFinish)
                .WillOnce([code](ugrpc::server::MiddlewareCallContext&, const std::optional<grpc::Status>& status) {
                    ASSERT_TRUE(status.has_value());
                    EXPECT_EQ(status->error_code(), code);
                });
        }
    }

    void ExpectUnexpectedExceptionLog(logging::Level level) {
        EXPECT_EQ(utest::GetSingleLog(GetLogCapture().Filter(kUnexpectedExceptionLog)).GetLevel(), level);
    }

    void RunCancelledCall() {
        UEXPECT_THROW(Client().SayHello(sample::ugrpc::GreetingRequest{}), ugrpc::client::CancelledError);
        GetServer().StopServing();
        ExpectUnexpectedExceptionLog(logging::Level::kWarning);
    }

    template <typename Failure>
    void CheckOnCallFinishFailure(Failure failure) {
        EXPECT_CALL(Service(), SayHello).Times(1);

        const testing::InSequence sequence;
        EXPECT_CALL(Middleware(kLastMiddleware), OnCallFinish)
            .WillOnce([failure](ugrpc::server::MiddlewareCallContext&, const std::optional<grpc::Status>& status) {
                ASSERT_TRUE(status.has_value());
                EXPECT_TRUE(status->ok());
                failure();
            });
        EXPECT_CALL(Middleware(kFirstMiddleware), OnCallFinish)
            .WillOnce([](ugrpc::server::MiddlewareCallContext&, const std::optional<grpc::Status>& status) {
                ASSERT_TRUE(status.has_value());
                EXPECT_TRUE(status->ok());
            });

        UEXPECT_NO_THROW(Client().SayHello(sample::ugrpc::GreetingRequest{}));
        GetServer().StopServing();
        EXPECT_EQ(
            utest::GetSingleLog(GetLogCapture().Filter(kUnexpectedOnCallFinishLog)).GetLevel(),
            logging::Level::kWarning
        );
    }
};

}  // namespace

UTEST_F(ServerMiddlewareUnexpectedExceptionsTest, CancelInHandler) {
    EXPECT_CALL(Service(), SayHello).WillOnce([](ugrpc::server::CallContext&, sample::ugrpc::GreetingRequest&&) {
        CancelCurrentTask();
        return sample::ugrpc::GreetingResponse{};
    });

    ExpectNoPreFinishHooks(kFirstMiddleware);
    ExpectNoPreFinishHooks(kLastMiddleware);
    ExpectCancellationCleanup();

    RunCancelledCall();
}

UTEST_F(ServerMiddlewareUnexpectedExceptionsTest, CancelInOnCallStart) {
    EXPECT_CALL(Service(), SayHello).Times(0);
    EXPECT_CALL(Middleware(kLastMiddleware), OnCallStart).WillOnce([](ugrpc::server::MiddlewareCallContext&) {
        CancelCurrentTask();
    });
    EXPECT_CALL(Middleware(kLastMiddleware), PostRecvMessage).Times(0);
    EXPECT_CALL(Middleware(kLastMiddleware), OnCallFinish).Times(0);
    EXPECT_CALL(Middleware(kFirstMiddleware), OnCallFinish(testing::_, testing::Eq(std::nullopt))).Times(1);
    ExpectNoPreFinishHooks(kFirstMiddleware);
    ExpectNoPreFinishHooks(kLastMiddleware);

    RunCancelledCall();
}

UTEST_F(ServerMiddlewareUnexpectedExceptionsTest, CancelInPostRecvMessage) {
    EXPECT_CALL(Service(), SayHello).Times(0);
    EXPECT_CALL(Middleware(kLastMiddleware), PostRecvMessage)
        .WillOnce([](ugrpc::server::MiddlewareCallContext&, google::protobuf::Message&) { CancelCurrentTask(); });
    ExpectNoPreFinishHooks(kFirstMiddleware);
    ExpectNoPreFinishHooks(kLastMiddleware);
    ExpectCancellationCleanup();

    RunCancelledCall();
}

UTEST_F(ServerMiddlewareUnexpectedExceptionsTest, CancelInPreSendMessage) {
    EXPECT_CALL(Service(), SayHello).Times(1);
    EXPECT_CALL(Middleware(kLastMiddleware), PreSendMessage)
        .WillOnce([](ugrpc::server::MiddlewareCallContext&, google::protobuf::Message&) { CancelCurrentTask(); });
    EXPECT_CALL(Middleware(kFirstMiddleware), PreSendMessage).Times(0);
    ExpectPreSendStatus(kFirstMiddleware, grpc::StatusCode::CANCELLED);
    ExpectPreSendStatus(kLastMiddleware, grpc::StatusCode::CANCELLED);
    ExpectFinishedWithStatus(grpc::StatusCode::CANCELLED);

    RunCancelledCall();
}

UTEST_F(ServerMiddlewareUnexpectedExceptionsTest, CancelInPreSendStatus) {
    EXPECT_CALL(Service(), SayHello).Times(1);
    EXPECT_CALL(Middleware(kLastMiddleware), PreSendMessage).Times(1);
    EXPECT_CALL(Middleware(kLastMiddleware), PreSendStatus)
        .WillOnce([](ugrpc::server::MiddlewareCallContext&, grpc::Status&) { CancelCurrentTask(); });
    EXPECT_CALL(Middleware(kFirstMiddleware), PreSendMessage).Times(0);
    ExpectPreSendStatus(kFirstMiddleware, grpc::StatusCode::CANCELLED);
    ExpectFinishedWithStatus(grpc::StatusCode::CANCELLED);

    RunCancelledCall();
}

UTEST_F(ServerMiddlewareUnexpectedExceptionsTest, NonStdExceptionInHandler) {
    EXPECT_CALL(Service(), SayHello)
        .WillOnce(
            [](ugrpc::server::CallContext&,
               sample::ugrpc::GreetingRequest&&) -> sample::ugrpc::UnitTestServiceBase::SayHelloResult {
                // NOLINTNEXTLINE(hicpp-exception-baseclass)
                throw UnexpectedHandlerException{};
            }
        );

    for (std::size_t index = 0; index < kCancellationMiddlewareCount; ++index) {
        EXPECT_CALL(Middleware(index), PreSendMessage).Times(0);
        ExpectPreSendStatus(index, grpc::StatusCode::UNKNOWN);
    }
    ExpectFinishedWithStatus(grpc::StatusCode::UNKNOWN);

    UEXPECT_THROW(Client().SayHello(sample::ugrpc::GreetingRequest{}), ugrpc::client::UnknownError);
    GetServer().StopServing();
    ExpectUnexpectedExceptionLog(logging::Level::kError);
}

UTEST_F(ServerMiddlewareUnexpectedExceptionsTest, NonStdExceptionInOnCallFinish) {
    CheckOnCallFinishFailure([] {
        // NOLINTNEXTLINE(hicpp-exception-baseclass)
        throw UnexpectedHandlerException{};
    });
}

UTEST_F(ServerMiddlewareUnexpectedExceptionsTest, CancelInOnCallFinish) { CheckOnCallFinishFailure(CancelCurrentTask); }

namespace {

class ResponseLifetimeService final : public ugrpc::server::GenericServiceBase {
public:
    std::atomic<bool> response_destroyed{false};

    GenericResult Handle(GenericCallContext&, GenericReaderWriter&) override {
        static std::string payload = [] {
            sample::ugrpc::GreetingResponse response;
            response.set_name("hello");
            return response.SerializeAsString();
        }();
        grpc::Slice slice{
            payload.data(),
            payload.size(),
            [](void* user_data) { static_cast<std::atomic<bool>*>(user_data)->store(true); },
            &response_destroyed
        };
        return grpc::ByteBuffer{&slice, 1};
    }
};

using ResponseLifetimeTest = tests::MiddlewaresFixture<
    tests::server::ServerMiddlewareBaseMock,
    ResponseLifetimeService,
    sample::ugrpc::UnitTestServiceClient,
    1>;

}  // namespace

UTEST_F(ResponseLifetimeTest, OnCallFinishBeforeResponseDestruction) {
    auto& response_destroyed = Service().response_destroyed;
    EXPECT_CALL(Middleware(), OnCallFinish)
        .WillOnce([&response_destroyed](ugrpc::server::MiddlewareCallContext&, const std::optional<grpc::Status>&) {
            EXPECT_FALSE(response_destroyed.load());
        });

    EXPECT_EQ(Client().SayHello(sample::ugrpc::GreetingRequest{}).name(), "hello");

    GetServer().StopServing();
    EXPECT_TRUE(response_destroyed.load());
}

USERVER_NAMESPACE_END
