#include <array>
#include <cstddef>
#include <string>
#include <utility>

#include <grpcpp/generic/async_generic_service.h>
#include <grpcpp/server_context.h>
#include <grpcpp/support/byte_buffer.h>
#include <grpcpp/support/server_callback.h>
#include <grpcpp/support/slice.h>
#include <grpcpp/support/status.h>

#include <userver/engine/deadline.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/ugrpc/byte_buffer_utils.hpp>
#include <userver/ugrpc/client/exceptions.hpp>
#include <userver/ugrpc/server/generic_service_base.hpp>
#include <userver/ugrpc/server/impl/async_methods.hpp>
#include <userver/ugrpc/server/impl/call_traits.hpp>
#include <userver/ugrpc/server/impl/callback_responder.hpp>
#include <userver/ugrpc/server/impl/responder.hpp>
#include <userver/ugrpc/tests/service.hpp>
#include <userver/ugrpc/tests/service_fixtures.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/algo.hpp>
#include <userver/utils/impl/internal_tag.hpp>

#include <tests/unit_test_client.usrv.pb.hpp>
#include <tests/unit_test_service.usrv.pb.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

constexpr int kMessageCount = 2;
constexpr int kFinalResponseNumber = 42;
constexpr char kMessageName[] = "backend-test";
constexpr char kUnaryCallName[] = "sample.ugrpc.UnitTestService/SayHello";
constexpr char kBidiCallName[] = "sample.ugrpc.UnitTestService/Chat";
constexpr char kUnexpectedMethodMessage[] = "Unexpected generic method";
constexpr char kCallbackApiName[] = "Callback";
constexpr char kCompletionQueueApiName[] = "CompletionQueue";
constexpr char kUnterminatedProtobufVarint[] = "\x80";
constexpr char kResponseMetadataKey[] = "backend-initial-metadata";
constexpr char kResponseMetadataValue[] = "sent-with-first-response";

grpc::ByteBuffer MakeTrackedBuffer(std::string& payload, std::size_t& destruction_count) {
    const grpc::Slice slice{
        payload.data(),
        payload.size(),
        [](void* user_data) { ++*static_cast<std::size_t*>(user_data); },
        &destruction_count
    };
    return grpc::ByteBuffer{&slice, 1};
}

void ReleaseDanglingBuffer(grpc::ByteBuffer& buffer, std::size_t payload_destruction_count) {
    if (payload_destruction_count != 0) {
        buffer.Release();
    }
}

void CheckServerApi(const grpc::ServerContextBase& context, bool use_callback_api) {
    EXPECT_EQ(dynamic_cast<const grpc::CallbackServerContext*>(&context) != nullptr, use_callback_api);
}

sample::ugrpc::StreamGreetingRequest MakeStreamRequest(int number) {
    sample::ugrpc::StreamGreetingRequest request;
    request.set_number(number);
    request.set_name(kMessageName);
    return request;
}

sample::ugrpc::StreamGreetingResponse MakeStreamResponse(int number) {
    sample::ugrpc::StreamGreetingResponse response;
    response.set_number(number);
    response.set_name(kMessageName);
    return response;
}

class TestWriteReactor final : public grpc::ServerWriteReactor<grpc::ByteBuffer> {
public:
    void OnDone() override {}
};

class DelayedBindWriter final : public grpc::ServerCallbackWriter<grpc::ByteBuffer> {
public:
    void Bind(TestWriteReactor& reactor) {
        reactor_ = &reactor;
        BindReactor(&reactor);
    }

    const grpc::ByteBuffer* GetCapturedResponse() const { return captured_response_; }

private:
    void Finish(grpc::Status) override {}
    void SendInitialMetadata() override {}
    void Write(const grpc::ByteBuffer*, grpc::WriteOptions) override {}
    void WriteAndFinish(const grpc::ByteBuffer* response, grpc::WriteOptions, grpc::Status) override {
        captured_response_ = response;
    }
    grpc::internal::ServerReactor* reactor() override { return reactor_; }
    void CallOnDone() override {}

    TestWriteReactor* reactor_{nullptr};
    const grpc::ByteBuffer* captured_response_{nullptr};
};

class CancelOnFinishReactor final {
public:
    explicit CancelOnFinishReactor(grpc::ServerContextBase& context)
        : context_(context)
    {}

    void Finish(const grpc::Status&) { CancelRpc(); }

    void StartWriteAndFinish(const grpc::ByteBuffer*, grpc::WriteOptions, const grpc::Status&) { CancelRpc(); }

private:
    void CancelRpc() {
        context_.TryCancel();
        const auto deadline = engine::Deadline::FromDuration(utest::kMaxTestWaitTime);
        while (!context_.IsCancelled() && !deadline.IsReached()) {
            engine::Yield();
        }
        EXPECT_TRUE(context_.IsCancelled());
    }

    grpc::ServerContextBase& context_;
};

struct CancelOnFinishTraits {
    using ReactorBase = CancelOnFinishReactor;
    using SerializedFinalResponse = grpc::ByteBuffer;
};

enum class FinishOperation { kWithResponse, kWithoutResponse, kWithError, kWriteAndFinish };

class CancelDuringFinishService final : public sample::ugrpc::UnitTestServiceBase {
public:
    explicit CancelDuringFinishService(FinishOperation operation)
        : operation_(operation)
    {}

    SayHelloResult SayHello(CallContext& context, sample::ugrpc::GreetingRequest&&) override {
        const engine::TaskCancellationBlocker cancellation_blocker;
        auto& server_context = context.GetServerContext();
        EXPECT_FALSE(server_context.IsCancelled());
        CancelOnFinishReactor reactor(server_context);
        grpc::ByteBuffer response;
        ugrpc::server::impl::CallbackResponder<CancelOnFinishTraits> responder(reactor, server_context, response);

        switch (operation_) {
            case FinishOperation::kWithResponse:
                EXPECT_FALSE(ugrpc::server::impl::Finish(responder, grpc::ByteBuffer{}, grpc::Status::OK));
                break;
            case FinishOperation::kWithoutResponse:
                EXPECT_FALSE(ugrpc::server::impl::Finish(responder, grpc::Status::OK));
                break;
            case FinishOperation::kWithError:
                EXPECT_FALSE(
                    ugrpc::server::impl::FinishWithError(responder, grpc::Status{grpc::StatusCode::INTERNAL, ""})
                );
                break;
            case FinishOperation::kWriteAndFinish:
                EXPECT_FALSE(ugrpc::server::impl::WriteAndFinish(
                    responder,
                    grpc::ByteBuffer{},
                    grpc::WriteOptions{},
                    grpc::Status::OK
                ));
                break;
        }
        return sample::ugrpc::GreetingResponse{};
    }

private:
    const FinishOperation operation_;
};

class BackendService final : public sample::ugrpc::UnitTestServiceBase {
public:
    explicit BackendService(bool use_callback_api)
        : use_callback_api_(use_callback_api)
    {}

    SayHelloResult SayHello(CallContext& context, sample::ugrpc::GreetingRequest&& request) override {
        CheckServerApi(context.GetServerContext(), use_callback_api_);
        sample::ugrpc::GreetingResponse response;
        response.set_name(request.name());
        return response;
    }

    ReadManyResult ReadMany(
        CallContext& context,
        sample::ugrpc::StreamGreetingRequest&& request,
        ReadManyWriter& writer
    ) override {
        CheckServerApi(context.GetServerContext(), use_callback_api_);
        context.GetServerContext().AddInitialMetadata(kResponseMetadataKey, kResponseMetadataValue);
        for (int number = 0; number < request.number(); ++number) {
            writer.Write(MakeStreamResponse(number));
        }
        return MakeStreamResponse(kFinalResponseNumber);
    }

    WriteManyResult WriteMany(CallContext& context, WriteManyReader& reader) override {
        CheckServerApi(context.GetServerContext(), use_callback_api_);
        sample::ugrpc::StreamGreetingRequest request;
        int count = 0;
        while (reader.Read(request)) {
            ++count;
        }
        return MakeStreamResponse(count);
    }

    ChatResult Chat(CallContext& context, ChatReaderWriter& stream) override {
        CheckServerApi(context.GetServerContext(), use_callback_api_);
        sample::ugrpc::StreamGreetingRequest request;
        while (stream.Read(request)) {
            stream.Write(MakeStreamResponse(request.number()));
        }
        return MakeStreamResponse(kFinalResponseNumber);
    }

private:
    const bool use_callback_api_;
};

class GenericBackendService final : public ugrpc::server::GenericServiceBase {
public:
    explicit GenericBackendService(bool use_callback_api)
        : use_callback_api_(use_callback_api)
    {}

    GenericResult Handle(GenericCallContext& context, GenericReaderWriter& stream) override {
        CheckServerApi(context.GetServerContext(), use_callback_api_);
        if (context.GetCallName() == kUnaryCallName) {
            grpc::ByteBuffer request_bytes;
            EXPECT_TRUE(stream.Read(request_bytes));
            sample::ugrpc::GreetingRequest request;
            EXPECT_TRUE(ugrpc::ParseFromByteBuffer(std::move(request_bytes), request));
            sample::ugrpc::GreetingResponse response;
            response.set_name(request.name());
            return ugrpc::SerializeToByteBuffer(response);
        }
        if (context.GetCallName() == kBidiCallName) {
            grpc::ByteBuffer request_bytes;
            while (stream.Read(request_bytes)) {
                sample::ugrpc::StreamGreetingRequest request;
                EXPECT_TRUE(ugrpc::ParseFromByteBuffer(std::move(request_bytes), request));
                stream.Write(ugrpc::SerializeToByteBuffer(MakeStreamResponse(request.number())));
            }
            return ugrpc::SerializeToByteBuffer(MakeStreamResponse(kFinalResponseNumber));
        }
        return grpc::Status{grpc::StatusCode::UNIMPLEMENTED, kUnexpectedMethodMessage};
    }

private:
    const bool use_callback_api_;
};

template <typename Service>
class BackendFixture
    : public ugrpc::tests::ServiceWithClientFixture<Service, sample::ugrpc::UnitTestServiceClient>,
      public testing::WithParamInterface<bool> {
    using Base = ugrpc::tests::ServiceWithClientFixture<Service, sample::ugrpc::UnitTestServiceClient>;

public:
    BackendFixture()
        : Base({.server_config = {.use_callback_api = GetParam()}}, std::in_place, GetParam())
    {}
};

using GrpcServerBackendTest = BackendFixture<BackendService>;
using GrpcGenericServerBackendTest = BackendFixture<GenericBackendService>;

class GrpcEmptyServerBackendTest : public ugrpc::tests::ServiceFixtureBase, public testing::WithParamInterface<bool> {
public:
    GrpcEmptyServerBackendTest()
        : ServiceFixtureBase(ugrpc::server::ServerConfig{.use_callback_api = GetParam()})
    {
        StartServer();
    }

    ~GrpcEmptyServerBackendTest() override { StopServer(); }
};

std::string GetApiName(const testing::TestParamInfo<bool>& info) {
    return info.param ? kCallbackApiName : kCompletionQueueApiName;
}

void CheckBidiExchange(const sample::ugrpc::UnitTestServiceClient& client) {
    auto stream = client.Chat();
    sample::ugrpc::StreamGreetingResponse response;
    for (int number = 0; number < kMessageCount; ++number) {
        EXPECT_TRUE(stream.Write(MakeStreamRequest(number)));
        ASSERT_TRUE(stream.Read(response));
        EXPECT_EQ(response.number(), number);
        EXPECT_EQ(response.name(), kMessageName);
    }
    EXPECT_TRUE(stream.WritesDone());
    ASSERT_TRUE(stream.Read(response));
    EXPECT_EQ(response.number(), kFinalResponseNumber);
    EXPECT_FALSE(stream.Read(response));
}

}  // namespace

UTEST(GrpcCallbackResponder, CancellationDuringFinishIsReported) {
    constexpr std::array kFinishOperations{
        FinishOperation::kWithResponse,
        FinishOperation::kWithoutResponse,
        FinishOperation::kWithError,
        FinishOperation::kWriteAndFinish,
    };
    for (const auto operation : kFinishOperations) {
        ugrpc::tests::Service<CancelDuringFinishService>
            service({.server_config = {.use_callback_api = true}}, std::in_place, operation);
        auto client = service.MakeClient<sample::ugrpc::UnitTestServiceClient>();
        UEXPECT_THROW(client.SayHello({}), ugrpc::client::CancelledError);
        service.GetServer().StopServing();
    }
}

UTEST(GrpcCallbackResponder, WriteAndFinishBufferOutlivesCaller) {
    using Traits = ugrpc::server::impl::CallbackCallTraits<
        ugrpc::server::impl::CallTraits<decltype(&sample::ugrpc::UnitTestServiceBase::ReadMany)>>;

    std::string payload = MakeStreamResponse(kFinalResponseNumber).SerializeAsString();
    std::size_t destruction_count = 0;
    {
        grpc::CallbackServerContext server_context;
        TestWriteReactor reactor;
        DelayedBindWriter writer;
        ugrpc::server::impl::CallbackResponder<Traits>
            responder{reactor, server_context, ugrpc::server::impl::no_final_response};
        {
            auto buffer = MakeTrackedBuffer(payload, destruction_count);
            EXPECT_TRUE(ugrpc::server::impl::WriteAndFinish(
                responder,
                std::move(buffer),
                grpc::WriteOptions{},
                grpc::Status::OK
            ));
        }
        ASSERT_EQ(destruction_count, 0);

        writer.Bind(reactor);
        ASSERT_NE(writer.GetCapturedResponse(), nullptr);
        sample::ugrpc::StreamGreetingResponse response;
        ASSERT_TRUE(ugrpc::ParseFromByteBuffer(grpc::ByteBuffer{*writer.GetCapturedResponse()}, response));
        EXPECT_EQ(response.number(), kFinalResponseNumber);
        EXPECT_EQ(response.name(), kMessageName);
        EXPECT_EQ(destruction_count, 0);
    }
    EXPECT_EQ(destruction_count, 1);
}

UTEST(GrpcServerDeserialization, ByteBufferOutlivesSource) {
    std::string payload = MakeStreamRequest(kMessageCount).SerializeAsString();
    std::size_t destruction_count = 0;
    grpc::ByteBuffer request_bytes;
    {
        auto buffer = MakeTrackedBuffer(payload, destruction_count);
        EXPECT_TRUE(ugrpc::impl::DeserializeMessage(std::move(buffer), request_bytes).ok());
    }

    ReleaseDanglingBuffer(request_bytes, destruction_count);
    ASSERT_EQ(destruction_count, 0);

    sample::ugrpc::StreamGreetingRequest request;
    ASSERT_TRUE(ugrpc::ParseFromByteBuffer(grpc::ByteBuffer{request_bytes}, request));
    EXPECT_EQ(request.name(), kMessageName);
    EXPECT_EQ(request.number(), kMessageCount);

    request_bytes.Clear();
    EXPECT_EQ(destruction_count, 1);
}

UTEST(GrpcServerDeserialization, ByteBufferReplacesPreviousPayload) {
    std::string previous_payload = MakeStreamRequest(kMessageCount).SerializeAsString();
    std::string payload = MakeStreamRequest(kFinalResponseNumber).SerializeAsString();
    std::size_t previous_destruction_count = 0;
    std::size_t destruction_count = 0;
    auto request_bytes = MakeTrackedBuffer(previous_payload, previous_destruction_count);
    {
        auto buffer = MakeTrackedBuffer(payload, destruction_count);
        EXPECT_TRUE(ugrpc::impl::DeserializeMessage(std::move(buffer), request_bytes).ok());
    }

    ReleaseDanglingBuffer(request_bytes, destruction_count);
    EXPECT_EQ(previous_destruction_count, 1);
    ASSERT_EQ(destruction_count, 0);

    sample::ugrpc::StreamGreetingRequest request;
    ASSERT_TRUE(ugrpc::ParseFromByteBuffer(grpc::ByteBuffer{request_bytes}, request));
    EXPECT_EQ(request.name(), kMessageName);
    EXPECT_EQ(request.number(), kFinalResponseNumber);

    request_bytes.Clear();
    EXPECT_EQ(previous_destruction_count, 1);
    EXPECT_EQ(destruction_count, 1);
}

UTEST(GrpcServerDeserialization, ProtobufReleasesPayload) {
    std::string payload = MakeStreamRequest(kMessageCount).SerializeAsString();
    std::size_t destruction_count = 0;
    {
        auto buffer = MakeTrackedBuffer(payload, destruction_count);
        sample::ugrpc::StreamGreetingRequest request;
        ASSERT_TRUE(ugrpc::impl::DeserializeMessage(std::move(buffer), request).ok());
        EXPECT_EQ(request.name(), kMessageName);
        EXPECT_EQ(request.number(), kMessageCount);
        EXPECT_EQ(destruction_count, 1);
    }
    EXPECT_EQ(destruction_count, 1);
}

UTEST(GrpcServerDeserialization, MalformedProtobufReleasesPayload) {
    std::string payload{kUnterminatedProtobufVarint};
    std::size_t destruction_count = 0;
    {
        auto buffer = MakeTrackedBuffer(payload, destruction_count);
        sample::ugrpc::StreamGreetingRequest request;
        const auto status = ugrpc::impl::DeserializeMessage(std::move(buffer), request);
        EXPECT_EQ(status.error_code(), grpc::StatusCode::INTERNAL);
        EXPECT_EQ(destruction_count, 1);
    }
    EXPECT_EQ(destruction_count, 1);
}

UTEST(GrpcServerDeserialization, InvalidProtobufBufferReturnsInternal) {
    grpc::ByteBuffer buffer;
    sample::ugrpc::StreamGreetingRequest request;
    const auto status = ugrpc::impl::DeserializeMessage(std::move(buffer), request);
    EXPECT_EQ(status.error_code(), grpc::StatusCode::INTERNAL);
}

UTEST(GrpcServerBackend, DefaultsToCompletionQueue) {
    ugrpc::tests::Service<BackendService> service(std::in_place, false);
    EXPECT_NE(service.GetServer().GetCompletionQueues(utils::impl::InternalTag{}), nullptr);
    auto client = service.MakeClient<sample::ugrpc::UnitTestServiceClient>();
    sample::ugrpc::GreetingRequest request;
    request.set_name(kMessageName);
    EXPECT_EQ(client.SayHello(request).name(), kMessageName);
}

UTEST_P(GrpcServerBackendTest, UnaryUsesSelectedBackend) {
    EXPECT_EQ(GetServer().GetCompletionQueues(utils::impl::InternalTag{}) == nullptr, GetParam());
    sample::ugrpc::GreetingRequest request;
    request.set_name(kMessageName);
    EXPECT_EQ(GetClient().SayHello(request).name(), kMessageName);
}

UTEST_P(GrpcServerBackendTest, ServerStreamingWithFinalResponse) {
    auto stream = GetClient().ReadMany(MakeStreamRequest(kMessageCount));
    sample::ugrpc::StreamGreetingResponse response;
    for (int number = 0; number < kMessageCount; ++number) {
        ASSERT_TRUE(stream.Read(response));
        EXPECT_EQ(response.number(), number);
    }
    EXPECT_EQ(
        utils::FindOptional(stream.GetContext().GetClientContext().GetServerInitialMetadata(), kResponseMetadataKey),
        kResponseMetadataValue
    );
    ASSERT_TRUE(stream.Read(response));
    EXPECT_EQ(response.number(), kFinalResponseNumber);
    EXPECT_FALSE(stream.Read(response));
}

UTEST_P(GrpcServerBackendTest, ClientStreaming) {
    auto stream = GetClient().WriteMany();
    for (int number = 0; number < kMessageCount; ++number) {
        EXPECT_TRUE(stream.Write(MakeStreamRequest(number)));
    }
    EXPECT_EQ(stream.Finish().number(), kMessageCount);
}

UTEST_P(GrpcServerBackendTest, BidirectionalStreamingWithFinalResponse) { CheckBidiExchange(GetClient()); }

UTEST_P(GrpcGenericServerBackendTest, Unary) {
    sample::ugrpc::GreetingRequest request;
    request.set_name(kMessageName);
    EXPECT_EQ(GetClient().SayHello(request).name(), kMessageName);
}

UTEST_P(GrpcGenericServerBackendTest, BidirectionalStreamingWithFinalResponse) { CheckBidiExchange(GetClient()); }

UTEST_P(GrpcEmptyServerBackendTest, StartsWithCompletionQueues) {
    EXPECT_NE(GetServer().GetCompletionQueues(utils::impl::InternalTag{}), nullptr);
    auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();
    UEXPECT_THROW(client.SayHello({}), ugrpc::client::UnimplementedError);
}

INSTANTIATE_UTEST_SUITE_P(BothApis, GrpcServerBackendTest, testing::Bool(), GetApiName);
INSTANTIATE_UTEST_SUITE_P(BothApis, GrpcGenericServerBackendTest, testing::Bool(), GetApiName);
INSTANTIATE_UTEST_SUITE_P(BothApis, GrpcEmptyServerBackendTest, testing::Bool(), GetApiName);

USERVER_NAMESPACE_END
