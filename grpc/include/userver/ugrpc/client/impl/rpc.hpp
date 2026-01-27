#pragma once

#include <utility>

#include <grpcpp/impl/codegen/proto_utils.h>
#include <grpcpp/support/async_stream.h>

#include <userver/utils/impl/internal_tag.hpp>

#include <userver/ugrpc/client/call_context.hpp>
#include <userver/ugrpc/client/impl/async_methods.hpp>
#include <userver/ugrpc/client/impl/async_stream_methods.hpp>
#include <userver/ugrpc/client/impl/call_state.hpp>
#include <userver/ugrpc/client/impl/middleware_pipeline.hpp>
#include <userver/ugrpc/client/impl/prepare_call.hpp>
#include <userver/ugrpc/client/stream_read_future.hpp>
#include <userver/ugrpc/time_utils.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

template <typename Reader, typename Response>
class StreamReadFutureImpl : public StreamReadFutureImplBase<Response> {
public:
    StreamReadFutureImpl(impl::StreamingCallState& state, Reader& reader, const Response& response) noexcept;

    StreamReadFutureImpl(StreamReadFutureImpl&&) = delete;
    StreamReadFutureImpl& operator=(StreamReadFutureImpl&&) = delete;

    ~StreamReadFutureImpl() override;

    bool IsReady() const noexcept override;

    bool Get() override;

private:
    impl::StreamingCallState& state_;
    Reader& reader_;
    const Response& response_{};
    bool valid_{};
};

template <typename Response>
class InputStream final {
public:
    template <typename Stub, typename Request>
    InputStream(
        CallParams&& params,
        PrepareServerStreamingCall<Stub, Request, Response> prepare_async_method,
        const Request& request
    );

    InputStream(InputStream&&) = delete;
    InputStream& operator=(InputStream&&) = delete;

    ~InputStream();

    CallContext& GetContext() noexcept { return context_; }
    const CallContext& GetContext() const noexcept { return context_; }

    [[nodiscard]] bool Read(Response& response);

private:
    StreamingCallState state_;
    CallContext context_;
    std::unique_ptr<grpc::ClientAsyncReader<Response>> stream_;
};

template <typename Request, typename Response>
class OutputStream final {
public:
    template <typename Stub>
    OutputStream(CallParams&& params, PrepareClientStreamingCall<Stub, Request, Response> prepare_async_method);

    OutputStream(OutputStream&&) = delete;
    OutputStream& operator=(OutputStream&&) = delete;

    ~OutputStream();

    CallContext& GetContext() noexcept { return context_; }
    const CallContext& GetContext() const noexcept { return context_; }

    [[nodiscard]] bool Write(const Request& request);

    void WriteAndCheck(const Request& request);

    Response Finish();

private:
    StreamingCallState state_;
    CallContext context_;
    Response response_;
    std::unique_ptr<grpc::ClientAsyncWriter<Request>> stream_;
};

template <typename Request, typename Response>
class BidirectionalStream final {
public:
    template <typename Stub>
    BidirectionalStream(CallParams&& params, PrepareBidiStreamingCall<Stub, Request, Response> prepare_async_method);

    BidirectionalStream(BidirectionalStream&&) = delete;
    BidirectionalStream& operator=(BidirectionalStream&&) = delete;

    ~BidirectionalStream();

    CallContext& GetContext() noexcept { return context_; }
    const CallContext& GetContext() const noexcept { return context_; }

    [[nodiscard]] bool Read(Response& response);

    StreamReadFuture<Response> ReadAsync(Response& response);

    [[nodiscard]] bool Write(const Request& request);

    void WriteAndCheck(const Request& request);

    [[nodiscard]] bool WritesDone();

private:
    StreamingCallState state_;
    CallContext context_;
    std::unique_ptr<grpc::ClientAsyncReaderWriter<Request, Response>> stream_;
};

template <typename Reader, typename Response>
StreamReadFutureImpl<Reader, Response>::StreamReadFutureImpl(
    impl::StreamingCallState& state,
    Reader& reader,
    const Response& response
) noexcept
        : state_{state}, reader_{reader}, response_{response}, valid_{true} {}

template <typename Reader, typename Response>
StreamReadFutureImpl<Reader, Response>::~StreamReadFutureImpl() {
    if (valid_) {
        // StreamReadFuture::Get wasn't called => finish RPC.
        impl::FinishAbandoned(reader_, state_);
    }
}

template <typename Reader, typename Response>
bool StreamReadFutureImpl<Reader, Response>::IsReady() const noexcept {
    UINVARIANT(valid_, "IsReady should be called only before 'Get'");
    return state_.GetAsyncMethodInvocation().IsReady();
}

template <typename Reader, typename Response>
bool StreamReadFutureImpl<Reader, Response>::Get() {
    UINVARIANT(valid_, "'Get' must be called only once");
    valid_ = false;
    const impl::StreamingCallState::AsyncMethodInvocationGuard guard(state_);
    const auto
        wait_status = impl::WaitAndTryCancelIfNeeded(state_.GetAsyncMethodInvocation(), state_.GetClientContext());
    if (wait_status == ugrpc::impl::AsyncMethodInvocation::WaitStatus::kCancelled) {
        state_.GetStatsScope().OnCancelled();
        state_.GetStatsScope().Flush();
    } else if (wait_status == ugrpc::impl::AsyncMethodInvocation::WaitStatus::kError) {
        // Finish can only be called once all the data is read, otherwise the
        // underlying gRPC driver hangs.
        impl::Finish(reader_, state_, /*final_response=*/nullptr, /*throw_on_error=*/true);
    } else {
        if constexpr (std::is_base_of_v<google::protobuf::Message, Response>) {
            RunMiddlewarePipeline(state_, impl::MiddlewareHooks::RecvMessageHooks(response_));
        }
    }
    return wait_status == ugrpc::impl::AsyncMethodInvocation::WaitStatus::kOk;
}

template <typename Response>
template <typename Stub, typename Request>
InputStream<Response>::InputStream(
    CallParams&& params,
    PrepareServerStreamingCall<Stub, Request, Response> prepare_async_method,
    const Request& request
)
    : state_{std::move(params)},
      context_{utils::impl::InternalTag{}, state_}
{
    RunMiddlewarePipeline(state_, MiddlewareHooks::StartCallHooks(ToBaseMessage(&request)));

    // NOLINTNEXTLINE(cppcoreguidelines-prefer-member-initializer)
    stream_ = impl::PrepareCall(
        prepare_async_method,
        state_.GetStub(),
        &state_.GetClientContext(),
        request,
        &state_.GetQueue()
    );
    impl::StartCall(*stream_, state_);

    state_.SetWritesFinished();
}

template <typename Response>
InputStream<Response>::~InputStream() {
    impl::FinishAbandoned(*stream_, state_);
}

template <typename Response>
bool InputStream<Response>::Read(Response& response) {
    if (!IsReadAvailable(state_)) {
        // If the stream is already finished, we must exit immediately.
        // If not, even the middlewares may access something that is already dead.
        return false;
    }

    if (impl::Read(*stream_, response, state_)) {
        RunMiddlewarePipeline(state_, MiddlewareHooks::RecvMessageHooks(response));
        return true;
    } else {
        // Finish can only be called once all the data is read, otherwise the
        // underlying gRPC driver hangs.
        impl::Finish(*stream_, state_, /*final_response=*/nullptr, /*throw_on_error=*/true);
        return false;
    }
}

template <typename Request, typename Response>
template <typename Stub>
OutputStream<Request, Response>::OutputStream(
    CallParams&& params,
    PrepareClientStreamingCall<Stub, Request, Response> prepare_async_method
)
    : state_{std::move(params)},
      context_{utils::impl::InternalTag{}, state_}
{
    RunMiddlewarePipeline(state_, MiddlewareHooks::StartCallHooks());

    // 'response_' will be filled upon successful 'Finish' async call
    // NOLINTNEXTLINE(cppcoreguidelines-prefer-member-initializer)
    stream_ = impl::PrepareCall(
        prepare_async_method,
        state_.GetStub(),
        &state_.GetClientContext(),
        &response_,
        &state_.GetQueue()
    );
    impl::StartCall(*stream_, state_);
}

template <typename Request, typename Response>
OutputStream<Request, Response>::~OutputStream() {
    impl::FinishAbandoned(*stream_, state_);
}

template <typename Request, typename Response>
bool OutputStream<Request, Response>::Write(const Request& request) {
    if (!IsWriteAvailable(state_)) {
        // If the stream is already finished, we must exit immediately.
        // If not, even the middlewares may access something that is already dead.
        return false;
    }

    RunMiddlewarePipeline(state_, MiddlewareHooks::SendMessageHooks(request));

    // Don't buffer writes, otherwise in an event subscription scenario, events
    // may never actually be delivered
    const grpc::WriteOptions write_options{};
    return impl::Write(*stream_, request, write_options, state_);
}

template <typename Request, typename Response>
void OutputStream<Request, Response>::WriteAndCheck(const Request& request) {
    if (!IsWriteAndCheckAvailable(state_)) {
        // If the stream is already finished, we must exit immediately.
        // If not, even the middlewares may access something that is already dead.
        throw RpcError(state_.GetCallName(), "'WriteAndCheck' called on a finished or closed stream");
    }

    RunMiddlewarePipeline(state_, MiddlewareHooks::SendMessageHooks(request));

    // Don't buffer writes, otherwise in an event subscription scenario, events
    // may never actually be delivered
    const grpc::WriteOptions write_options{};
    if (!impl::Write(*stream_, request, write_options, state_)) {
        // We don't need final_response here, because the RPC is broken anyway.
        impl::Finish(*stream_, state_, /*final_response=*/nullptr, /*throw_on_error=*/true);
    }
}

template <typename Request, typename Response>
Response OutputStream<Request, Response>::Finish() {
    // gRPC does not implicitly call `WritesDone` in `Finish`,
    // contrary to the documentation
    if (IsWriteAvailable(state_)) {
        impl::WritesDone(*stream_, state_);
    }

    impl::Finish(*stream_, state_, ToBaseMessage(&response_), /*throw_on_error=*/true);

    return std::move(response_);
}

template <typename Request, typename Response>
template <typename Stub>
BidirectionalStream<Request, Response>::BidirectionalStream(
    CallParams&& params,
    PrepareBidiStreamingCall<Stub, Request, Response> prepare_async_method
)
    : state_{std::move(params)},
      context_{utils::impl::InternalTag{}, state_}
{
    RunMiddlewarePipeline(state_, MiddlewareHooks::StartCallHooks());

    // NOLINTNEXTLINE(cppcoreguidelines-prefer-member-initializer)
    stream_ = impl::PrepareCall(prepare_async_method, state_.GetStub(), &state_.GetClientContext(), &state_.GetQueue());
    impl::StartCall(*stream_, state_);
}

template <typename Request, typename Response>
BidirectionalStream<Request, Response>::~BidirectionalStream() {
    impl::FinishAbandoned(*stream_, state_);
}

template <typename Request, typename Response>
StreamReadFuture<Response> BidirectionalStream<Request, Response>::ReadAsync(Response& response) {
    if (!IsReadAvailable(state_)) {
        // If the stream is already finished, we must exit immediately.
        // If not, even the middlewares may access something that is already dead.
        throw RpcError(state_.GetCallName(), "'ReadAsync' called on a finished call");
    }

    impl::ReadAsync(*stream_, response, state_);
    using BidirectionalStreamReadFutureImpl =
        StreamReadFutureImpl<grpc::ClientAsyncReaderWriter<Request, Response>, Response>;
    return StreamReadFuture<Response>{std::make_unique<BidirectionalStreamReadFutureImpl>(state_, *stream_, response)};
}

template <typename Request, typename Response>
bool BidirectionalStream<Request, Response>::Read(Response& response) {
    if (!IsReadAvailable(state_)) {
        return false;
    }

    auto future = ReadAsync(response);
    return future.Get();
}

template <typename Request, typename Response>
bool BidirectionalStream<Request, Response>::Write(const Request& request) {
    if (!IsWriteAvailable(state_)) {
        // If the stream is already finished, we must exit immediately.
        // If not, even the middlewares may access something that is already dead.
        return false;
    }

    {
        const auto lock = state_.TakeMutexIfBidirectional();
        if (state_.IsFinished()) {
            // It't forbidden to work with a stream after Finish.
            return false;
        }
        RunMiddlewarePipeline(state_, MiddlewareHooks::SendMessageHooks(request));
    }

    // Don't buffer writes, optimize for ping-pong-style interaction
    const grpc::WriteOptions write_options{};
    return impl::Write(*stream_, request, write_options, state_);
}

template <typename Request, typename Response>
void BidirectionalStream<Request, Response>::WriteAndCheck(const Request& request) {
    if (!IsWriteAndCheckAvailable(state_)) {
        // If the stream is already finished, we must exit immediately.
        // If not, even the middlewares may access something that is already dead.
        throw RpcError(state_.GetCallName(), "'WriteAndCheck' called on a finished or closed stream");
    }

    {
        const auto lock = state_.TakeMutexIfBidirectional();
        if (state_.IsFinished()) {
            // It't forbidden to work with a stream after Finish.
            throw ugrpc::client::RpcInterruptedError{state_.GetCallName(), "WriteAndCheck"};
        }
        RunMiddlewarePipeline(state_, MiddlewareHooks::SendMessageHooks(request));
    }

    // Don't buffer writes, optimize for ping-pong-style interaction
    const grpc::WriteOptions write_options{};
    impl::WriteAndCheck(*stream_, request, write_options, state_);
}

template <typename Request, typename Response>
bool BidirectionalStream<Request, Response>::WritesDone() {
    if (!IsWriteAvailable(state_)) {
        // If the stream is already finished, we must exit immediately.
        // If not, even the middlewares may access something that is already dead.
        return false;
    }

    return impl::WritesDone(*stream_, state_);
}

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
