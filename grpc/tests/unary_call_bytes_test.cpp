#include <userver/utest/utest.hpp>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <grpcpp/client_context.h>
#include <grpcpp/impl/serialization_traits.h>
#include <grpcpp/support/byte_buffer.h>
#include <grpcpp/support/config.h>
#include <grpcpp/support/slice.h>
#include <grpcpp/support/status.h>

#include <userver/engine/task/cancel.hpp>
#include <userver/ugrpc/byte_buffer_utils.hpp>
#include <userver/ugrpc/client/call_options.hpp>
#include <userver/ugrpc/client/exceptions.hpp>
#include <userver/ugrpc/client/generic_client.hpp>
#include <userver/ugrpc/client/impl/call_params.hpp>
#include <userver/ugrpc/client/impl/client_data_accessor.hpp>
#include <userver/ugrpc/client/impl/unary_call.hpp>
#include <userver/ugrpc/client/response_future.hpp>
#include <userver/ugrpc/server/generic_service_base.hpp>
#include <userver/ugrpc/tests/service_fixtures.hpp>

#include <tests/client_middleware_base_gmock.hpp>
#include <tests/middlewares_fixture.hpp>
#include <tests/proto2_service_client.usrv.pb.hpp>
#include <tests/unit_test_client.usrv.pb.hpp>
#include <tests/unit_test_service.usrv.pb.hpp>
#include <tests/unit_test_service_gmock.hpp>

USERVER_NAMESPACE_BEGIN

namespace tests {

struct UnaryRequestSerializationState {
    std::atomic<std::size_t> count{0};
    std::function<void()> on_serialize;
    grpc::Status status;
};

struct TrackedUnaryRequest {
    sample::ugrpc::GreetingRequest payload;
    std::shared_ptr<UnaryRequestSerializationState> state{std::make_shared<UnaryRequestSerializationState>()};
};

}  // namespace tests

USERVER_NAMESPACE_END

namespace grpc {

template <>
class SerializationTraits<USERVER_NAMESPACE::tests::TrackedUnaryRequest> {
public:
    static Status Serialize(
        const USERVER_NAMESPACE::tests::TrackedUnaryRequest& request,
        ByteBuffer* buffer,
        bool* own_buffer
    ) {
        *own_buffer = false;
        auto& state = *request.state;
        ++state.count;
        if (state.on_serialize) {
            state.on_serialize();
        }
        if (!state.status.ok()) {
            return state.status;
        }
        *buffer = USERVER_NAMESPACE::ugrpc::SerializeToByteBuffer(request.payload);
        *own_buffer = true;
        return Status::OK;
    }
};

}  // namespace grpc

USERVER_NAMESPACE_BEGIN

namespace {

using Request = sample::ugrpc::GreetingRequest;
using Response = sample::ugrpc::GreetingResponse;

constexpr std::size_t kFailedAttempts = 2;
constexpr std::size_t kAttempts = kFailedAttempts + 1;
constexpr std::size_t kLargeNameSize = 100000;
constexpr std::string_view kSayHelloCallName = "sample.ugrpc.UnitTestService/SayHello";
constexpr std::string_view kSerializationError = "Request serialization failed";
constexpr auto kExpiredContextDeadline = std::chrono::system_clock::time_point{};

Request MakeRequest() {
    Request request;
    request.set_name(std::string(kLargeNameSize, 'q'));
    return request;
}

Response MakeResponse() {
    Response response;
    response.set_name(std::string(kLargeNameSize, 'r'));
    return response;
}

grpc::ByteBuffer MakeUnparsableResponseBytes() {
    constexpr std::string_view kTruncatedLengthDelimitedField = "\x0a\x7f";
    const grpc::Slice slice{kTruncatedLengthDelimitedField.data(), kTruncatedLengthDelimitedField.size()};
    return grpc::ByteBuffer{&slice, 1};
}

class UnparsableResponseGenericService final : public ugrpc::server::GenericServiceBase {
public:
    GenericResult Handle(GenericCallContext& /*context*/, GenericReaderWriter& stream) override {
        grpc::ByteBuffer request_bytes;
        EXPECT_TRUE(stream.Read(request_bytes));
        return MakeUnparsableResponseBytes();
    }
};

grpc::ByteBuffer MakePartiallyParsableResponseBytes() {
    constexpr std::string_view kValidNameFieldThenTruncatedGreetingField = "\x0a\x03yyy\x12\x7f";
    const grpc::Slice
        slice{kValidNameFieldThenTruncatedGreetingField.data(), kValidNameFieldThenTruncatedGreetingField.size()};
    return grpc::ByteBuffer{&slice, 1};
}

class PartiallyParsableThenEmptyResponseGenericService final : public ugrpc::server::GenericServiceBase {
public:
    GenericResult Handle(GenericCallContext& /*context*/, GenericReaderWriter& stream) override {
        grpc::ByteBuffer request_bytes;
        EXPECT_TRUE(stream.Read(request_bytes));
        if (calls_.fetch_add(1) == 0) {
            return MakePartiallyParsableResponseBytes();
        }
        return grpc::Status::OK;
    }

private:
    std::atomic<std::size_t> calls_{0};
};

class NoResponseGenericService final : public ugrpc::server::GenericServiceBase {
public:
    GenericResult Handle(GenericCallContext& /*context*/, GenericReaderWriter& stream) override {
        grpc::ByteBuffer request_bytes;
        EXPECT_TRUE(stream.Read(request_bytes));
        return grpc::Status::OK;
    }
};

class EmptyResponseGenericService final : public ugrpc::server::GenericServiceBase {
public:
    GenericResult Handle(GenericCallContext&, GenericReaderWriter& stream) override {
        grpc::ByteBuffer request_bytes;
        if (!stream.Read(request_bytes)) {
            constexpr std::string_view kMissingRequestMessage = "Request message is missing";
            return grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, grpc::string{kMissingRequestMessage}};
        }
        const grpc::Slice empty_slice{std::size_t{0}};
        return grpc::ByteBuffer{&empty_slice, 1};
    }
};

class ResponseThenErrorGenericService final : public ugrpc::server::GenericServiceBase {
public:
    GenericResult Handle(GenericCallContext&, GenericReaderWriter& stream) override {
        grpc::ByteBuffer request_bytes;
        EXPECT_TRUE(stream.Read(request_bytes));
        if (calls_.fetch_add(1) == 0) {
            constexpr std::string_view kRetryMessage = "Retry after receiving a response message";
            stream.Write(ugrpc::SerializeToByteBuffer(MakeResponse()));
            return grpc::Status{grpc::StatusCode::UNAVAILABLE, grpc::string{kRetryMessage}};
        }
        return grpc::Status::OK;
    }

    std::size_t GetCalls() const { return calls_.load(); }

private:
    std::atomic<std::size_t> calls_{0};
};

using UnparsableResponseTest =
    ugrpc::tests::ServiceWithClientFixture<UnparsableResponseGenericService, sample::ugrpc::UnitTestServiceClient>;

using NoResponseTest =
    ugrpc::tests::ServiceWithClientFixture<NoResponseGenericService, sample::ugrpc::UnitTestServiceClient>;

using EmptyResponseTest =
    ugrpc::tests::ServiceWithClientFixture<EmptyResponseGenericService, sample::ugrpc::UnitTestServiceClient>;

using EmptyProto2ResponseTest =
    ugrpc::tests::ServiceWithClientFixture<EmptyResponseGenericService, sample::ugrpc::Proto2TestServiceClient>;

using ResponseThenErrorTest =
    ugrpc::tests::ServiceWithClientFixture<ResponseThenErrorGenericService, sample::ugrpc::UnitTestServiceClient>;

using PartiallyParsableThenEmptyResponseTest = ugrpc::tests::ServiceWithClientFixture<
    PartiallyParsableThenEmptyResponseGenericService,
    sample::ugrpc::UnitTestServiceClient>;

using RetryBytesTest = tests::MiddlewaresFixture<
    tests::client::ClientMiddlewareBaseMock,
    ::testing::NiceMock<tests::UnitTestServiceGmock>,
    sample::ugrpc::UnitTestServiceClient,
    /*N=*/1>;

using UnaryRequestSerialization = RetryBytesTest;

tests::TrackedUnaryRequest MakeTrackedRequest() { return {MakeRequest()}; }

ugrpc::client::impl::CallParams MakeSerializationCallParams(
    const sample::ugrpc::UnitTestServiceClient& client,
    ugrpc::client::CallOptions options = {}
) {
    constexpr std::size_t kSayHelloMethodId = 0;
    return ugrpc::client::impl::CreateCallParams(
        ugrpc::client::impl::ClientDataAccessor::GetClientData(client),
        kSayHelloMethodId,
        std::move(options)
    );
}

ugrpc::client::impl::UnaryCall<tests::TrackedUnaryRequest, Response> MakeSerializationCall(
    const sample::ugrpc::UnitTestServiceClient& client,
    const tests::TrackedUnaryRequest& request,
    ugrpc::client::CallOptions options = {}
) {
    return {MakeSerializationCallParams(client, std::move(options)), request};
}

}  // namespace

UTEST_F(UnparsableResponseTest, ThrowsInterruptedError) {
    UEXPECT_THROW(GetClient().SayHello(MakeRequest()), ugrpc::client::RpcInterruptedError);
}

UTEST_F(NoResponseTest, OkStatusWithoutMessageYieldsEmptyResponse) {
    Response response;
    UEXPECT_NO_THROW(response = GetClient().SayHello(MakeRequest()));

    EXPECT_EQ(response.ByteSizeLong(), std::size_t{0});
}

UTEST_F(EmptyResponseTest, EmptyMessageYieldsEmptyResponse) {
    const auto response = GetClient().SayHello(MakeRequest());
    EXPECT_EQ(response.ByteSizeLong(), std::size_t{0});
}

UTEST_F(EmptyResponseTest, GenericClientPreservesEmptyMessage) {
    const auto client = MakeClient<ugrpc::client::GenericClient>();
    auto response_bytes = client.UnaryCall(kSayHelloCallName, ugrpc::SerializeToByteBuffer(MakeRequest()));
    ASSERT_TRUE(response_bytes.Valid());
    EXPECT_EQ(response_bytes.Length(), std::size_t{0});

    Response response;
    EXPECT_TRUE(ugrpc::ParseFromByteBuffer(std::move(response_bytes), response));
}

UTEST_F(EmptyResponseTest, AsyncGenericClientPreservesEmptyMessage) {
    const auto client = MakeClient<ugrpc::client::GenericClient>();
    auto future = client.AsyncUnaryCall(kSayHelloCallName, ugrpc::SerializeToByteBuffer(MakeRequest()));
    auto response_bytes = future.Get();
    ASSERT_TRUE(response_bytes.Valid());
    EXPECT_EQ(response_bytes.Length(), std::size_t{0});
}

UTEST_F(EmptyResponseTest, GenericClientCanForwardEmptyMessage) {
    constexpr auto kForwardTimeout = std::chrono::seconds{1};
    const auto client = MakeClient<ugrpc::client::GenericClient>();
    auto response_bytes = client.UnaryCall(kSayHelloCallName, ugrpc::SerializeToByteBuffer(MakeRequest()));
    ugrpc::client::CallOptions call_options;
    call_options.SetTimeout(kForwardTimeout);
    UEXPECT_NO_THROW(client.UnaryCall(kSayHelloCallName, response_bytes, std::move(call_options)));
}

UTEST_F(EmptyProto2ResponseTest, MissingRequiredResponseFieldIsRejected) {
    constexpr std::string_view kRequestName = "request";
    sample::ugrpc::Proto2Request request;
    request.set_name(std::string{kRequestName});
    UEXPECT_THROW(GetClient().SayHello(request), ugrpc::client::RpcInterruptedError);
}

UTEST_F(PartiallyParsableThenEmptyResponseTest, RetryDoesNotLeakPartiallyParsedResponse) {
    constexpr int kRetryAttempts = 2;
    ugrpc::client::CallOptions call_options;
    call_options.SetAttempts(kRetryAttempts);

    Response response;
    UEXPECT_NO_THROW(response = GetClient().SayHello(MakeRequest(), std::move(call_options)));

    EXPECT_EQ(response.ByteSizeLong(), std::size_t{0});
}

UTEST_F(RetryBytesTest, AllAttemptsSendIdenticalRequest) {
    std::vector<std::string> received_requests;
    ON_CALL(Service(), SayHello).WillByDefault([&received_requests](ugrpc::server::CallContext&, Request&& request) {
        received_requests.push_back(request.SerializeAsString());
        if (received_requests.size() <= kFailedAttempts) {
            return tests::UnitTestServiceGmock::SayHelloResult{grpc::Status{grpc::StatusCode::UNAVAILABLE, "retry me"}};
        }
        return tests::UnitTestServiceGmock::SayHelloResult{MakeResponse()};
    });

    ugrpc::client::CallOptions call_options;
    call_options.SetAttempts(static_cast<int>(kAttempts));
    UEXPECT_NO_THROW(Client().SayHello(MakeRequest(), std::move(call_options)));

    const std::vector<std::string> expected_requests(kAttempts, MakeRequest().SerializeAsString());
    EXPECT_EQ(received_requests, expected_requests);
}

UTEST_F(ResponseThenErrorTest, RetryDoesNotReuseResponseFromFailedAttempt) {
    constexpr int kRetryAttempts = 2;
    ugrpc::client::CallOptions options;
    options.SetAttempts(kRetryAttempts);
    const auto response = GetClient().SayHello(MakeRequest(), std::move(options));
    EXPECT_EQ(GetService().GetCalls(), kRetryAttempts);
    EXPECT_EQ(response.ByteSizeLong(), std::size_t{0});
}

UTEST_F(ResponseThenErrorTest, GenericRetryDoesNotReuseResponseFromFailedAttempt) {
    constexpr int kRetryAttempts = 2;
    const auto client = MakeClient<ugrpc::client::GenericClient>();
    ugrpc::client::CallOptions options;
    options.SetAttempts(kRetryAttempts);
    const auto
        response = client.UnaryCall(kSayHelloCallName, ugrpc::SerializeToByteBuffer(MakeRequest()), std::move(options));
    EXPECT_EQ(GetService().GetCalls(), kRetryAttempts);
    EXPECT_FALSE(response.Valid());
}

UTEST_F(UnaryRequestSerialization, CancellationBeforeWorkerStartSkipsSerialization) {
    EXPECT_CALL(Middleware(), PreStartCall).Times(0);
    EXPECT_CALL(Service(), SayHello).Times(0);
    const auto request = MakeTrackedRequest();
    ugrpc::client::ResponseFuture<Response> future{MakeSerializationCallParams(Client()), request};
    future.Cancel();

    UEXPECT_THROW((void)future.Get(), ugrpc::client::RpcCancelledError);
    EXPECT_EQ(request.state->count.load(), std::size_t{0});
}

UTEST_F(UnaryRequestSerialization, CancellationDuringContextCreationCancelsCall) {
    EXPECT_CALL(Middleware(), PreStartCall).Times(1);
    ON_CALL(Service(), SayHello).WillByDefault([](ugrpc::server::CallContext&, Request&&) { return MakeResponse(); });
    const auto request = MakeTrackedRequest();
    ugrpc::client::CallOptions options;
    options.SetClientContextFactory([] {
        engine::current_task::RequestCancel();
        return std::make_unique<grpc::ClientContext>();
    });
    auto call = MakeSerializationCall(Client(), request, std::move(options));

    UEXPECT_THROW(call.Perform(), ugrpc::client::RpcCancelledError);
    EXPECT_EQ(request.state->count.load(), std::size_t{1});
}

UTEST_F(UnaryRequestSerialization, CancellationDuringSerializationSkipsStartHooks) {
    EXPECT_CALL(Middleware(), PreStartCall).Times(0);
    EXPECT_CALL(Service(), SayHello).Times(0);
    const auto request = MakeTrackedRequest();
    request.state->on_serialize = [] { engine::current_task::RequestCancel(); };
    auto call = MakeSerializationCall(Client(), request);

    UEXPECT_THROW(call.Perform(), ugrpc::client::RpcCancelledError);
    EXPECT_EQ(request.state->count.load(), std::size_t{1});
}

UTEST_F(UnaryRequestSerialization, SerializationErrorTakesPrecedenceOverCancellation) {
    EXPECT_CALL(Middleware(), PreStartCall).Times(0);
    EXPECT_CALL(Service(), SayHello).Times(0);
    const auto request = MakeTrackedRequest();
    request.state->status = {grpc::StatusCode::INTERNAL, grpc::string{kSerializationError}};
    request.state->on_serialize = [] { engine::current_task::RequestCancel(); };
    auto call = MakeSerializationCall(Client(), request);

    UEXPECT_THROW(call.Perform(), ugrpc::client::InternalError);
    EXPECT_EQ(request.state->count.load(), std::size_t{1});
}

UTEST_F(UnaryRequestSerialization, SerializationErrorDoesNotStartCall) {
    EXPECT_CALL(Middleware(), PreStartCall).Times(0);
    EXPECT_CALL(Service(), SayHello).Times(0);
    const auto request = MakeTrackedRequest();
    request.state->status = {grpc::StatusCode::INTERNAL, grpc::string{kSerializationError}};
    auto call = MakeSerializationCall(Client(), request);

    UEXPECT_THROW(call.Perform(), ugrpc::client::InternalError);
    EXPECT_EQ(request.state->count.load(), std::size_t{1});
}

UTEST_F(UnaryRequestSerialization, SerializesOnceAcrossRetries) {
    std::atomic<std::size_t> calls{0};
    ON_CALL(Service(), SayHello).WillByDefault([&calls](ugrpc::server::CallContext&, Request&&) {
        if (calls.fetch_add(1) < kFailedAttempts) {
            constexpr std::string_view kRetryMessage = "Retry the request";
            return tests::UnitTestServiceGmock::SayHelloResult{
                grpc::Status{grpc::StatusCode::UNAVAILABLE, grpc::string{kRetryMessage}}
            };
        }
        return tests::UnitTestServiceGmock::SayHelloResult{MakeResponse()};
    });
    const auto request = MakeTrackedRequest();
    ugrpc::client::CallOptions options;
    options.SetAttempts(static_cast<int>(kAttempts));
    auto call = MakeSerializationCall(Client(), request, std::move(options));

    EXPECT_EQ(call.Perform().SerializeAsString(), MakeResponse().SerializeAsString());
    EXPECT_EQ(calls.load(), kAttempts);
    EXPECT_EQ(request.state->count.load(), std::size_t{1});
}

UTEST_F(UnaryRequestSerialization, ExpiredContextDeadlineFailsCall) {
    constexpr int kSingleAttempt = 1;
    EXPECT_CALL(Service(), SayHello).Times(0);
    const auto request = MakeTrackedRequest();
    ugrpc::client::CallOptions options;
    options.SetAttempts(kSingleAttempt);
    options.SetClientContextFactory([] {
        auto context = std::make_unique<grpc::ClientContext>();
        context->set_deadline(kExpiredContextDeadline);
        return context;
    });
    auto call = MakeSerializationCall(Client(), request, std::move(options));

    UEXPECT_THROW(call.Perform(), ugrpc::client::DeadlineExceededError);
    EXPECT_EQ(request.state->count.load(), std::size_t{1});
}

UTEST_F(UnaryRequestSerialization, RetryAfterExpiredContextDeadlineDoesNotReserialize) {
    constexpr int kRetryAttempts = 2;
    std::atomic<std::size_t> contexts{0};
    EXPECT_CALL(Service(), SayHello).WillOnce([](ugrpc::server::CallContext&, Request&&) { return MakeResponse(); });
    const auto request = MakeTrackedRequest();
    ugrpc::client::CallOptions options;
    options.SetAttempts(kRetryAttempts);
    options.SetClientContextFactory([&contexts] {
        auto context = std::make_unique<grpc::ClientContext>();
        if (contexts.fetch_add(1) == 0) {
            context->set_deadline(kExpiredContextDeadline);
        }
        return context;
    });
    auto call = MakeSerializationCall(Client(), request, std::move(options));

    EXPECT_EQ(call.Perform().SerializeAsString(), MakeResponse().SerializeAsString());
    EXPECT_EQ(contexts.load(), kRetryAttempts);
    EXPECT_EQ(request.state->count.load(), std::size_t{1});
}

USERVER_NAMESPACE_END
