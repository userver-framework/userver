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

class UnitTestServiceMock : public sample::ugrpc::UnitTestServiceBase {
public:
    MOCK_METHOD(ChatResult, Chat, (ugrpc::server::CallContext& /*context*/, ChatReaderWriter& /*stream*/), (override));
};

using ChatReaderWriter = UnitTestServiceMock::ChatReaderWriter;

struct Flags final {
    bool set_error{true};
};

class ServerMiddlewareHooksStreamingTest : public tests::MiddlewaresFixture<
                                               tests::server::ServerMiddlewareBaseMock,
                                               ::testing::NiceMock<UnitTestServiceMock>,
                                               sample::ugrpc::UnitTestServiceClient,
                                               /*N=*/3> {
protected:
    void SetSuccessHandler() {
        ON_CALL(Service(), Chat).WillByDefault([](ugrpc::server::CallContext& /*context*/, ChatReaderWriter& bs) {
            sample::ugrpc::StreamGreetingRequest request;
            sample::ugrpc::StreamGreetingResponse response;
            while (bs.Read(request)) {
                response.set_number(request.number());
                response.set_name("Hello, " + request.name());
                bs.Write(response);
            }
            return grpc::Status::OK;
        });
    }

    template <typename Handler>
    void SetHandler(Handler&& handler) {
        ON_CALL(Service(), Chat)
            .WillByDefault([handler = std::forward<Handler>(handler
                            )](ugrpc::server::CallContext& /*context*/, ChatReaderWriter& bs) { return handler(bs); });
    }

    const MiddlewareMock& M0() { return Middleware(0); }
    const MiddlewareMock& M1() { return Middleware(1); }
    const MiddlewareMock& M2() { return Middleware(2); }

    void SetupRpcFinishWait(engine::SingleUseEvent& rpc_finish) {
        ON_CALL(M0(), OnCallFinish)
            .WillByDefault([&rpc_finish](ugrpc::server::MiddlewareCallContext&, const grpc::Status&) {
                // 'OnCallFinish' of M0 is the last hook => M1 and M2 hooks already finished.
                rpc_finish.Send();
            });
    }

    void WaitRpcFinish(engine::SingleUseEvent& rpc_finish) {
        // We have to explicit wait when middlewares hooks are finished.
        const auto status = rpc_finish.WaitUntil(engine::Deadline::FromDuration(utest::kMaxTestWaitTime));
        EXPECT_EQ(engine::FutureStatus::kReady, status);
    }
};

// NOLINTNEXTLINE(fuchsia-multiple-inheritance)
class ServerMiddlewareHooksStreamingWithParamTest : public ServerMiddlewareHooksStreamingTest,
                                                    public testing::WithParamInterface<Flags> {
protected:
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

UTEST_F(ServerMiddlewareHooksStreamingTest, SuccessBasic) {
    SetSuccessHandler();
    engine::SingleUseEvent rpc_finish;
    SetupRpcFinishWait(rpc_finish);

    EXPECT_CALL(M1(), OnCallStart).Times(1);
    EXPECT_CALL(M1(), PostRecvMessage).Times(1);
    EXPECT_CALL(M1(), PreSendMessage).Times(1);
    EXPECT_CALL(M1(), OnCallFinish).Times(1);

    EXPECT_CALL(M2(), OnCallStart).Times(1);
    EXPECT_CALL(M2(), PostRecvMessage).Times(1);
    EXPECT_CALL(M2(), PreSendMessage).Times(1);
    EXPECT_CALL(M2(), OnCallFinish).Times(1);

    const auto& client = Client();

    auto bs = client.Chat();

    sample::ugrpc::StreamGreetingRequest out{};
    out.set_name("userver");
    out.set_number(1);
    UEXPECT_NO_THROW(bs.WriteAndCheck(out));
    EXPECT_TRUE(bs.WritesDone());

    sample::ugrpc::StreamGreetingResponse in;
    EXPECT_TRUE(bs.Read(in));
    EXPECT_EQ("Hello, userver", in.name());
    EXPECT_EQ(1, in.number());

    WaitRpcFinish(rpc_finish);
}

UTEST_F(ServerMiddlewareHooksStreamingTest, SuccessMany) {
    static const std::size_t kOnServerRecvCount = 7;
    static const std::size_t kOnServerSendCount = 9;

    SetHandler([](ChatReaderWriter& bs) {
        for (std::size_t i = 0; i < kOnServerRecvCount; ++i) {
            sample::ugrpc::StreamGreetingRequest in;
            EXPECT_TRUE(bs.Read(in));
        }
        for (std::size_t i = 0; i < kOnServerSendCount; ++i) {
            sample::ugrpc::StreamGreetingResponse out;
            out.set_name("userver 42");
            out.set_number(42);
            bs.Write(out);
        }
        return grpc::Status::OK;
    });
    engine::SingleUseEvent rpc_finish;
    SetupRpcFinishWait(rpc_finish);

    EXPECT_CALL(M1(), OnCallStart).Times(1);
    EXPECT_CALL(M1(), PostRecvMessage).Times(kOnServerRecvCount);
    EXPECT_CALL(M1(), PreSendMessage).Times(kOnServerSendCount);
    EXPECT_CALL(M1(), OnCallFinish).Times(1);

    EXPECT_CALL(M2(), OnCallStart).Times(1);
    EXPECT_CALL(M2(), PostRecvMessage).Times(kOnServerRecvCount);
    EXPECT_CALL(M2(), PreSendMessage).Times(kOnServerSendCount);
    EXPECT_CALL(M2(), OnCallFinish).Times(1);

    const auto& client = Client();

    auto bs = client.Chat();

    for (std::size_t i = 0; i < kOnServerRecvCount; ++i) {
        sample::ugrpc::StreamGreetingRequest out{};
        out.set_name("userver");
        out.set_number(i);
        EXPECT_TRUE(bs.Write(out));
    }
    EXPECT_TRUE(bs.WritesDone());

    for (std::size_t i = 0; i < kOnServerSendCount; ++i) {
        sample::ugrpc::StreamGreetingResponse in;
        EXPECT_TRUE(bs.Read(in));
        EXPECT_EQ("userver 42", in.name());
        EXPECT_EQ(42, in.number());
    }
    WaitRpcFinish(rpc_finish);
}

UTEST_P(ServerMiddlewareHooksStreamingWithParamTest, FailInFirstMiddlewareOnStart) {
    SetSuccessHandler();
    engine::SingleUseEvent rpc_finish;
    SetupRpcFinishWait(rpc_finish);

    engine::SingleUseEvent client_finished;
    ON_CALL(M1(), OnCallStart).WillByDefault([this, &client_finished](ugrpc::server::MiddlewareCallContext& context) {
        EXPECT_EQ(
            client_finished.WaitUntil(engine::Deadline::FromDuration(utest::kMaxTestWaitTime)),
            engine::FutureStatus::kReady
        );
        return SetErrorOrThrowRuntimeError(context);
    });
    EXPECT_CALL(M1(), OnCallStart).Times(1);
    EXPECT_CALL(M1(), PostRecvMessage).Times(0);
    EXPECT_CALL(M1(), PreSendMessage).Times(0);
    EXPECT_CALL(M1(), OnCallFinish).Times(0);

    EXPECT_CALL(M2(), OnCallStart).Times(0);
    EXPECT_CALL(M2(), PostRecvMessage).Times(0);
    EXPECT_CALL(M2(), PreSendMessage).Times(0);
    EXPECT_CALL(M2(), OnCallFinish).Times(0);

    const auto& client = Client();

    auto bs = client.Chat();

    sample::ugrpc::StreamGreetingRequest out{};
    out.set_name("userver");
    out.set_number(1);
    UEXPECT_NO_THROW(bs.WriteAndCheck(out));
    client_finished.Send();

    WaitRpcFinish(rpc_finish);

    sample::ugrpc::StreamGreetingResponse in;
    UEXPECT_THROW_MSG(
        [[maybe_unused]] const auto success = bs.Read(in),
        ugrpc::client::UnknownError,
        kUnknownErrorStatus.error_message()
    );
}

UTEST_P(ServerMiddlewareHooksStreamingWithParamTest, FailInSecondMiddlewareOnStart) {
    SetSuccessHandler();
    engine::SingleUseEvent rpc_finish;
    SetupRpcFinishWait(rpc_finish);

    engine::SingleUseEvent client_finished;
    ON_CALL(M2(), OnCallStart).WillByDefault([this, &client_finished](ugrpc::server::MiddlewareCallContext& context) {
        EXPECT_EQ(
            client_finished.WaitUntil(engine::Deadline::FromDuration(utest::kMaxTestWaitTime)),
            engine::FutureStatus::kReady
        );
        return SetErrorOrThrowRuntimeError(context);
    });
    EXPECT_CALL(M1(), OnCallStart).Times(1);
    EXPECT_CALL(M1(), PostRecvMessage).Times(0);
    EXPECT_CALL(M1(), PreSendMessage).Times(0);
    // OnCallStart of M1 is successfully => OnCallFinish must be called.
    EXPECT_CALL(M1(), OnCallFinish).Times(1);

    EXPECT_CALL(M2(), OnCallStart).Times(1);
    EXPECT_CALL(M2(), PostRecvMessage).Times(0);
    EXPECT_CALL(M2(), PreSendMessage).Times(0);
    EXPECT_CALL(M2(), OnCallFinish).Times(0);

    const auto& client = Client();

    auto bs = client.Chat();

    sample::ugrpc::StreamGreetingRequest out{};
    out.set_name("userver");
    out.set_number(1);
    UEXPECT_NO_THROW(bs.WriteAndCheck(out));
    client_finished.Send();

    WaitRpcFinish(rpc_finish);

    sample::ugrpc::StreamGreetingResponse in;
    UEXPECT_THROW_MSG(
        [[maybe_unused]] const auto success = bs.Read(in),
        ugrpc::client::UnknownError,
        kUnknownErrorStatus.error_message()
    );
}

UTEST_P(ServerMiddlewareHooksStreamingWithParamTest, FailInFirstMiddlewareOnPostRecvMessage) {
    SetSuccessHandler();
    engine::SingleUseEvent rpc_finish;
    SetupRpcFinishWait(rpc_finish);

    ON_CALL(M1(), PostRecvMessage)
        .WillByDefault([this](ugrpc::server::MiddlewareCallContext& context, google::protobuf::Message&) {
            return SetErrorOrThrowRuntimeError(context);
        });
    EXPECT_CALL(M1(), OnCallStart).Times(1);
    EXPECT_CALL(M1(), PostRecvMessage).Times(1);
    EXPECT_CALL(M1(), PreSendMessage).Times(0);
    // OnCallStart of M1 is successfully => OnCallFinish must be called.
    EXPECT_CALL(M1(), OnCallFinish).Times(1);

    EXPECT_CALL(M2(), OnCallStart).Times(1);
    EXPECT_CALL(M2(), PostRecvMessage).Times(0);
    EXPECT_CALL(M2(), PreSendMessage).Times(0);
    EXPECT_CALL(M2(), OnCallFinish).Times(1);

    const auto& client = Client();

    auto bs = client.Chat();

    sample::ugrpc::StreamGreetingRequest out{};
    out.set_name("userver");
    out.set_number(1);
    UEXPECT_NO_THROW(bs.WriteAndCheck(out));
    EXPECT_TRUE(bs.WritesDone());

    sample::ugrpc::StreamGreetingResponse in;
    UEXPECT_THROW_MSG(
        [[maybe_unused]] const auto success = bs.Read(in),
        ugrpc::client::UnknownError,
        kUnknownErrorStatus.error_message()
    );
    WaitRpcFinish(rpc_finish);
}

UTEST_P(ServerMiddlewareHooksStreamingWithParamTest, FailInSecondMiddlewareOnPostRecvMessage) {
    SetSuccessHandler();
    engine::SingleUseEvent rpc_finish;
    SetupRpcFinishWait(rpc_finish);

    std::size_t count = 0;
    ON_CALL(M2(), PostRecvMessage)
        .WillByDefault([this, &count](ugrpc::server::MiddlewareCallContext& context, google::protobuf::Message&) {
            if (count == 1) {
                return SetErrorOrThrowRuntimeError(context);
            }
            ++count;
        });
    EXPECT_CALL(M1(), OnCallStart).Times(1);
    EXPECT_CALL(M1(), PostRecvMessage).Times(2);
    EXPECT_CALL(M1(), PreSendMessage).Times(1);
    EXPECT_CALL(M1(), OnCallFinish).Times(1);

    EXPECT_CALL(M2(), OnCallStart).Times(1);
    EXPECT_CALL(M2(), PostRecvMessage).Times(2);
    EXPECT_CALL(M2(), PreSendMessage).Times(1);
    EXPECT_CALL(M2(), OnCallFinish).Times(1);

    const auto& client = Client();

    auto bs = client.Chat();

    sample::ugrpc::StreamGreetingRequest out{};
    out.set_name("userver");
    out.set_number(1);
    UEXPECT_NO_THROW(bs.WriteAndCheck(out));

    sample::ugrpc::StreamGreetingResponse in;
    EXPECT_TRUE(bs.Read(in));
    EXPECT_EQ(in.name(), "Hello, userver");
    EXPECT_EQ(in.number(), 1);

    UEXPECT_NO_THROW(bs.WriteAndCheck(out));
    EXPECT_TRUE(bs.WritesDone());

    UEXPECT_THROW_MSG(
        [[maybe_unused]] const auto success = bs.Read(in),
        ugrpc::client::UnknownError,
        kUnknownErrorStatus.error_message()
    );
    WaitRpcFinish(rpc_finish);
}

UTEST_P(ServerMiddlewareHooksStreamingWithParamTest, FailInSecondMiddlewareOnPreSendMessage) {
    SetSuccessHandler();
    engine::SingleUseEvent rpc_finish;
    SetupRpcFinishWait(rpc_finish);

    constexpr std::size_t kSuccessCount = 17;
    std::size_t count = 0;
    ON_CALL(M2(), PreSendMessage)
        .WillByDefault([this, &count](ugrpc::server::MiddlewareCallContext& context, google::protobuf::Message&) {
            if (count == kSuccessCount) {
                return SetErrorOrThrowRuntimeError(context);
            }
            ++count;
        });
    EXPECT_CALL(M1(), OnCallStart).Times(1);
    EXPECT_CALL(M1(), PostRecvMessage).Times(kSuccessCount + 1);
    EXPECT_CALL(M1(), PreSendMessage).Times(kSuccessCount);
    EXPECT_CALL(M1(), OnCallFinish).Times(1);

    EXPECT_CALL(M2(), OnCallStart).Times(1);
    EXPECT_CALL(M2(), PostRecvMessage).Times(kSuccessCount + 1);
    EXPECT_CALL(M2(), PreSendMessage).Times(kSuccessCount + 1);
    EXPECT_CALL(M2(), OnCallFinish).Times(1);

    const auto& client = Client();

    auto bs = client.Chat();

    for (std::size_t i = 0; i < kSuccessCount; ++i) {
        const std::string suffix = std::to_string(i);
        sample::ugrpc::StreamGreetingRequest out{};
        out.set_name("userver" + suffix);
        out.set_number(i);
        UEXPECT_NO_THROW(bs.WriteAndCheck(out));

        sample::ugrpc::StreamGreetingResponse in;
        EXPECT_TRUE(bs.Read(in));
        EXPECT_EQ(in.name(), "Hello, userver" + suffix);
        EXPECT_EQ(in.number(), i);
    }

    sample::ugrpc::StreamGreetingRequest out{};
    out.set_name("userver");
    out.set_number(0);
    UEXPECT_NO_THROW(bs.WriteAndCheck(out));
    EXPECT_TRUE(bs.WritesDone());

    sample::ugrpc::StreamGreetingResponse in;
    UEXPECT_THROW_MSG(
        [[maybe_unused]] const auto success = bs.Read(in),
        ugrpc::client::UnknownError,
        kUnknownErrorStatus.error_message()
    );
    WaitRpcFinish(rpc_finish);
}

UTEST_F(ServerMiddlewareHooksStreamingTest, ThrowInHandler) {
    engine::SingleUseEvent client_finished;
    SetHandler([&client_finished](ChatReaderWriter& bs) {
        sample::ugrpc::StreamGreetingRequest request;

        EXPECT_TRUE(bs.Read(request));

        sample::ugrpc::StreamGreetingResponse response;
        response.set_number(request.number());
        response.set_name("Hello, " + request.name());
        bs.Write(response);

        EXPECT_TRUE(bs.Read(request));
        EXPECT_EQ(
            client_finished.WaitUntil(engine::Deadline::FromDuration(utest::kMaxTestWaitTime)),
            engine::FutureStatus::kReady
        );

        throw std::runtime_error{"fail :("};

        return grpc::Status::OK;
    });
    engine::SingleUseEvent rpc_finish;
    SetupRpcFinishWait(rpc_finish);

    EXPECT_CALL(M1(), OnCallStart).Times(1);
    EXPECT_CALL(M1(), PostRecvMessage).Times(2);
    EXPECT_CALL(M1(), PreSendMessage).Times(1);
    EXPECT_CALL(M1(), OnCallFinish).Times(1);

    EXPECT_CALL(M2(), OnCallStart).Times(1);
    EXPECT_CALL(M2(), PostRecvMessage).Times(2);
    EXPECT_CALL(M2(), PreSendMessage).Times(1);
    EXPECT_CALL(M2(), OnCallFinish).Times(1);

    const auto& client = Client();

    auto bs = client.Chat();

    for (std::size_t i = 0; i < 2; ++i) {
        sample::ugrpc::StreamGreetingRequest out{};
        out.set_name("userver");
        out.set_number(i);
        UEXPECT_NO_THROW(bs.WriteAndCheck(out));
    }
    EXPECT_TRUE(bs.WritesDone());

    sample::ugrpc::StreamGreetingResponse in;
    EXPECT_TRUE(bs.Read(in));
    client_finished.Send();

    UEXPECT_THROW_MSG(
        [[maybe_unused]] const auto success = bs.Read(in),
        ugrpc::client::UnknownError,
        kUnknownErrorStatus.error_message()
    );
    WaitRpcFinish(rpc_finish);
}

INSTANTIATE_UTEST_SUITE_P(
    /*no prefix*/,
    ServerMiddlewareHooksStreamingWithParamTest,
    testing::Values(Flags{/*set_error=*/true}, Flags{/*set_error*/ false})
);

USERVER_NAMESPACE_END
