#include <memory>
#include <optional>
#include <utility>

#include <grpcpp/generic/async_generic_service.h>
#include <grpcpp/security/auth_context.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <grpcpp/support/byte_buffer.h>
#include <grpcpp/support/status.h>

#include <userver/engine/deadline.hpp>
#include <userver/engine/single_consumer_event.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/ugrpc/byte_buffer_utils.hpp>
#include <userver/ugrpc/client/call_options.hpp>
#include <userver/ugrpc/client/exceptions.hpp>
#include <userver/ugrpc/server/generic_service_base.hpp>
#include <userver/ugrpc/server/impl/context_allocator.hpp>
#include <userver/ugrpc/server/middlewares/base.hpp>
#include <userver/ugrpc/tests/service_fixtures.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/algo.hpp>

#include <tests/unit_test_client.usrv.pb.hpp>
#include <tests/unit_test_service.usrv.pb.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

constexpr char kMetadataKey[] = "context-lifetime";
constexpr char kMetadataValue[] = "available-after-on-done";
constexpr char kErrorRequest[] = "error";
constexpr char kCancelBeforeFinishMetadata[] = "cancel-before-finish";
constexpr char kEnabledMetadataValue[] = "1";

struct ContextLifetimeState final {
    engine::SingleConsumerEvent grpc_released;
    engine::SingleConsumerEvent finish_hook_entered;
    std::weak_ptr<const grpc::AuthContext> auth_context;
    std::optional<grpc::Status> finish_status;
};

class NotifyingContextAllocator final : public grpc::ContextAllocator {
public:
    explicit NotifyingContextAllocator(std::shared_ptr<ContextLifetimeState> state)
        : state_(std::move(state))
    {}

    grpc::CallbackServerContext* NewCallbackServerContext() override { return allocator_.NewCallbackServerContext(); }

    grpc::GenericCallbackServerContext* NewGenericCallbackServerContext() override {
        return allocator_.NewGenericCallbackServerContext();
    }

    void Release(grpc::CallbackServerContext* context) override {
        allocator_.Release(context);
        state_->grpc_released.Send();
    }

    void Release(grpc::GenericCallbackServerContext* context) override {
        allocator_.Release(context);
        state_->grpc_released.Send();
    }

private:
    ugrpc::server::impl::ContextAllocator allocator_;
    const std::shared_ptr<ContextLifetimeState> state_;
};

class ContextLifetimeMiddleware final : public ugrpc::server::MiddlewareBase {
public:
    explicit ContextLifetimeMiddleware(std::shared_ptr<ContextLifetimeState> state)
        : state_(std::move(state))
    {}

    void OnCallStart(ugrpc::server::MiddlewareCallContext& context) const override {
        // Only ServerContext retains the AuthContext wrapper, so a weak reference observes its destruction.
        state_->auth_context = context.GetServerContext().auth_context();
        ASSERT_FALSE(state_->auth_context.expired());
    }

    void OnCallFinish(ugrpc::server::MiddlewareCallContext& context, const std::optional<grpc::Status>& status)
        const override {
        const engine::TaskCancellationBlocker cancellation_blocker;
        state_->finish_status = status;
        state_->finish_hook_entered.Send();
        ASSERT_TRUE(state_->grpc_released.WaitForEventFor(utest::kMaxTestWaitTime));
        ASSERT_FALSE(state_->auth_context.expired()) << "ServerContext was destroyed before OnCallFinish completed";

        const auto& server_context = context.GetServerContext();
        EXPECT_EQ(utils::FindOptional(server_context.client_metadata(), kMetadataKey), kMetadataValue);
        EXPECT_FALSE(server_context.peer().empty());
    }

private:
    const std::shared_ptr<ContextLifetimeState> state_;
};

class ContextLifetimeService final : public sample::ugrpc::UnitTestServiceBase {
public:
    SayHelloResult SayHello(CallContext&, sample::ugrpc::GreetingRequest&& request) override {
        if (request.name() == kErrorRequest) {
            return grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, kErrorRequest};
        }
        return sample::ugrpc::GreetingResponse{};
    }

    ReadManyResult ReadMany(CallContext&, sample::ugrpc::StreamGreetingRequest&&, ReadManyWriter& writer) override {
        writer.Write(sample::ugrpc::StreamGreetingResponse{});
        return grpc::Status::OK;
    }

    WriteManyResult WriteMany(CallContext&, WriteManyReader& reader) override {
        sample::ugrpc::StreamGreetingRequest request;
        EXPECT_TRUE(reader.Read(request));
        EXPECT_FALSE(reader.Read(request));
        return sample::ugrpc::StreamGreetingResponse{};
    }

    ChatResult Chat(CallContext&, ChatReaderWriter& stream) override {
        sample::ugrpc::StreamGreetingRequest request;
        EXPECT_TRUE(stream.Read(request));
        EXPECT_FALSE(stream.Read(request));
        stream.Write(sample::ugrpc::StreamGreetingResponse{});
        return grpc::Status::OK;
    }
};

class GenericContextLifetimeService final : public ugrpc::server::GenericServiceBase {
public:
    GenericResult Handle(GenericCallContext&, GenericReaderWriter& stream) override {
        grpc::ByteBuffer request;
        EXPECT_TRUE(stream.Read(request));
        return ugrpc::SerializeToByteBuffer(sample::ugrpc::GreetingResponse{});
    }
};

class CancellationMiddleware final : public ugrpc::server::MiddlewareBase {
public:
    void PreSendStatus(ugrpc::server::MiddlewareCallContext& context, grpc::Status&) const override {
        auto& server_context = context.GetServerContext();
        if (utils::FindOptional(server_context.client_metadata(), kCancelBeforeFinishMetadata)) {
            const engine::TaskCancellationBlocker cancellation_blocker;
            server_context.TryCancel();
            const auto deadline = engine::Deadline::FromDuration(utest::kMaxTestWaitTime);
            while (!server_context.IsCancelled() && !deadline.IsReached()) {
                engine::Yield();
            }
            ASSERT_TRUE(server_context.IsCancelled());
        }
    }
};

template <typename Service>
class CallbackContextFixture : public ugrpc::tests::ServiceFixtureBase {
protected:
    CallbackContextFixture()
        : ServiceFixtureBase(ugrpc::server::ServerConfig{.use_callback_api = true})
    {
        GetServer().WithServerBuilder([this](grpc::ServerBuilder& builder) {
            builder.SetContextAllocator(std::make_unique<NotifyingContextAllocator>(state_));
        });
        SetServerMiddlewares(
            {std::make_shared<ContextLifetimeMiddleware>(state_), std::make_shared<CancellationMiddleware>()}
        );
        RegisterService(service_);
        StartServer();
        client_.emplace(MakeClient<sample::ugrpc::UnitTestServiceClient>());
    }

    ~CallbackContextFixture() override {
        client_.reset();
        StopServer();
    }

    auto& Client() { return *client_; }

    static ugrpc::client::CallOptions MakeCallOptions() {
        ugrpc::client::CallOptions options;
        options.AddMetadata(kMetadataKey, kMetadataValue);
        return options;
    }

    void CheckContextReleased(std::optional<grpc::StatusCode> expected_status = grpc::StatusCode::OK) {
        ASSERT_TRUE(state_->finish_hook_entered.WaitForEventFor(utest::kMaxTestWaitTime));
        GetServer().StopServing();
        EXPECT_TRUE(state_->auth_context.expired()) << "ServerContext leaked after the handler completed";
        const auto actual_status =
            state_->finish_status ? std::make_optional(state_->finish_status->error_code()) : std::nullopt;
        EXPECT_EQ(actual_status, expected_status);
    }

private:
    const std::shared_ptr<ContextLifetimeState> state_{std::make_shared<ContextLifetimeState>()};
    Service service_;
    std::optional<sample::ugrpc::UnitTestServiceClient> client_;
};

using GrpcCallbackContextTest = CallbackContextFixture<ContextLifetimeService>;
using GrpcGenericCallbackContextTest = CallbackContextFixture<GenericContextLifetimeService>;

}  // namespace

UTEST_F(GrpcCallbackContextTest, Unary) {
    UEXPECT_NO_THROW(Client().SayHello({}, MakeCallOptions()));
    CheckContextReleased();
}

UTEST_F(GrpcCallbackContextTest, UnaryError) {
    sample::ugrpc::GreetingRequest request;
    request.set_name(kErrorRequest);
    UEXPECT_THROW(Client().SayHello(request, MakeCallOptions()), ugrpc::client::InvalidArgumentError);
    CheckContextReleased(grpc::StatusCode::INVALID_ARGUMENT);
}

UTEST_F(GrpcCallbackContextTest, ServerStreaming) {
    auto stream = Client().ReadMany({}, MakeCallOptions());
    sample::ugrpc::StreamGreetingResponse response;
    EXPECT_TRUE(stream.Read(response));
    EXPECT_FALSE(stream.Read(response));
    CheckContextReleased();
}

UTEST_F(GrpcCallbackContextTest, ClientStreaming) {
    auto stream = Client().WriteMany(MakeCallOptions());
    EXPECT_TRUE(stream.Write({}));
    UEXPECT_NO_THROW(stream.Finish());
    CheckContextReleased();
}

UTEST_F(GrpcCallbackContextTest, BidiStreaming) {
    auto stream = Client().Chat(MakeCallOptions());
    EXPECT_TRUE(stream.Write({}));
    EXPECT_TRUE(stream.WritesDone());
    sample::ugrpc::StreamGreetingResponse response;
    EXPECT_TRUE(stream.Read(response));
    EXPECT_FALSE(stream.Read(response));
    CheckContextReleased();
}

UTEST_F(GrpcGenericCallbackContextTest, Unary) {
    UEXPECT_NO_THROW(Client().SayHello({}, MakeCallOptions()));
    CheckContextReleased();
}

UTEST_F(GrpcCallbackContextTest, UnaryCancellationBeforeFinish) {
    auto options = MakeCallOptions();
    options.AddMetadata(kCancelBeforeFinishMetadata, kEnabledMetadataValue);
    UEXPECT_THROW(Client().SayHello({}, std::move(options)), ugrpc::client::CancelledError);
    CheckContextReleased(std::nullopt);
}

UTEST_F(GrpcCallbackContextTest, UnaryErrorCancellationBeforeFinish) {
    sample::ugrpc::GreetingRequest request;
    request.set_name(kErrorRequest);
    auto options = MakeCallOptions();
    options.AddMetadata(kCancelBeforeFinishMetadata, kEnabledMetadataValue);
    UEXPECT_THROW(Client().SayHello(request, std::move(options)), ugrpc::client::CancelledError);
    CheckContextReleased(std::nullopt);
}

UTEST_F(GrpcCallbackContextTest, ServerStreamingCancellationBeforeFinish) {
    auto options = MakeCallOptions();
    options.AddMetadata(kCancelBeforeFinishMetadata, kEnabledMetadataValue);
    auto stream = Client().ReadMany({}, std::move(options));
    sample::ugrpc::StreamGreetingResponse response;
    UEXPECT_THROW(while (stream.Read(response)){}, ugrpc::client::CancelledError);
    CheckContextReleased(std::nullopt);
}

UTEST_F(GrpcGenericCallbackContextTest, UnaryCancellationBeforeFinish) {
    auto options = MakeCallOptions();
    options.AddMetadata(kCancelBeforeFinishMetadata, kEnabledMetadataValue);
    UEXPECT_THROW(Client().SayHello({}, std::move(options)), ugrpc::client::CancelledError);
    CheckContextReleased(std::nullopt);
}

USERVER_NAMESPACE_END
