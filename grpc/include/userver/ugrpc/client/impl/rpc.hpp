#pragma once

/// @file userver/ugrpc/client/impl/rpc.hpp
/// @brief Classes representing an outgoing RPC

#include <utility>

#include <grpcpp/impl/codegen/proto_utils.h>

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

/// @brief Controls a single request -> response stream RPC
///
/// This class is not thread-safe except for `GetContext`.
///
/// The RPC is cancelled on destruction unless the stream is closed (`Read` has
/// returned `false`). In that case the connection is not closed (it will be
/// reused for new RPCs), and the server receives `RpcInterruptedError`
/// immediately. gRPC provides no way to early-close a server-streaming RPC
/// gracefully.
template <typename Response>
class [[nodiscard]] InputStream final {
public:
    template <typename Stub, typename Request>
    InputStream(
        CallParams&& params,
        PrepareServerStreamingCall<Stub, Request, Response> prepare_async_method,
        const Request& request
    );

    ~InputStream();

    InputStream(InputStream&&) = delete;
    InputStream& operator=(InputStream&&) = delete;

    CallContext& GetContext() noexcept { return context_; }
    const CallContext& GetContext() const noexcept { return context_; }

    /// @brief Await and read the next incoming message
    ///
    /// On end-of-input, `Finish` is called automatically.
    ///
    /// @param response where to put response on success
    /// @returns `true` on success, `false` on end-of-input, task cancellation,
    //           or if the stream is already closed for reads
    /// @throws ugrpc::client::RpcError on an RPC error
    [[nodiscard]] bool Read(Response& response);

private:
    StreamingCallState state_;
    CallContext context_;
    RawReader<Response> stream_;
};

/// @brief Controls a request stream -> single response RPC
///
/// This class is not thread-safe except for `GetContext`.
///
/// The RPC is cancelled on destruction unless `Finish` has been called. In that
/// case the connection is not closed (it will be reused for new RPCs), and the
/// server receives `RpcInterruptedError` immediately.
template <typename Request, typename Response>
class [[nodiscard]] OutputStream final {
public:
    template <typename Stub>
    OutputStream(CallParams&& params, PrepareClientStreamingCall<Stub, Request, Response> prepare_async_method);

    ~OutputStream();

    OutputStream(OutputStream&&) = delete;
    OutputStream& operator=(OutputStream&&) = delete;

    CallContext& GetContext() noexcept { return context_; }
    const CallContext& GetContext() const noexcept { return context_; }

    /// @brief Write the next outgoing message
    ///
    /// `Write` doesn't store any references to `request`, so it can be
    /// deallocated right after the call.
    ///
    /// @param request the next message to write
    /// @return true if the data is going to the wire; false if the write
    ///         operation failed (including due to task cancellation,
    //          or if the stream is already closed for writes),
    ///         in which case no more writes will be accepted,
    ///         and the error details can be fetched from Finish
    [[nodiscard]] bool Write(const Request& request);

    /// @brief Write the next outgoing message and check result
    ///
    /// `WriteAndCheck` doesn't store any references to `request`, so it can be
    /// deallocated right after the call.
    ///
    /// `WriteAndCheck` verifies result of the write and generates exception
    /// in case of issues.
    ///
    /// @param request the next message to write
    /// @throws ugrpc::client::RpcError on an RPC error
    /// @throws ugrpc::client::RpcCancelledError on task cancellation
    /// @throws ugrpc::client::RpcError if the stream is already closed for writes
    void WriteAndCheck(const Request& request);

    /// @brief Complete the RPC successfully
    ///
    /// Should be called once all the data is written. The server will then
    /// send a single `Response`.
    ///
    /// `Finish` should not be called multiple times.
    ///
    /// The connection is not closed, it will be reused for new RPCs.
    ///
    /// @returns the single `Response` received after finishing the writes
    /// @throws ugrpc::client::RpcError on an RPC error
    /// @throws ugrpc::client::RpcCancelledError on task cancellation
    Response Finish();

private:
    StreamingCallState state_;
    CallContext context_;
    Response response_;
    RawWriter<Request> stream_;
};

/// @brief Controls a request stream -> response stream RPC
///
/// It is safe to call the following methods from different coroutines:
///
///   - `GetContext`;
///   - one of (`Read`, `ReadAsync`);
///   - one of (`Write`, `WritesDone`).
///
/// `WriteAndCheck` is NOT thread-safe.
///
/// The RPC is cancelled on destruction unless the stream is closed (`Read` has
/// returned `false`). In that case the connection is not closed (it will be
/// reused for new RPCs), and the server receives `RpcInterruptedError`
/// immediately. gRPC provides no way to early-close a server-streaming RPC
/// gracefully.
///
/// `Read` and `AsyncRead` can throw if error status is received from server.
/// User MUST NOT call `Read` or `AsyncRead` again after failure of any of these
/// operations.
///
/// `Write` and `WritesDone` methods do not throw, but indicate issues with
/// the RPC by returning `false`.
///
/// `WriteAndCheck` is intended for ping-pong scenarios, when after write
/// operation the user calls `Read` and vice versa.
///
/// If `Write` or `WritesDone` returns negative result, the user MUST NOT call
/// any of these methods anymore.
/// Instead the user SHOULD call `Read` method until the end of input. If
/// `Write` or `WritesDone` finishes with negative result, finally `Read`
/// will throw an exception.
/// ## Usage example:
///
/// @snippet grpc/tests/stream_test.cpp concurrent bidirectional stream
///
template <typename Request, typename Response>
class [[nodiscard]] BidirectionalStream final {
public:
    using RawStream = grpc::ClientAsyncReaderWriter<Request, Response>;
    using StreamReadFuture = ugrpc::client::StreamReadFuture<RawStream>;

    template <typename Stub>
    BidirectionalStream(CallParams&& params, PrepareBidiStreamingCall<Stub, Request, Response> prepare_async_method);

    ~BidirectionalStream();

    BidirectionalStream(BidirectionalStream&&) = delete;
    BidirectionalStream& operator=(BidirectionalStream&&) = delete;

    CallContext& GetContext() noexcept { return context_; }
    const CallContext& GetContext() const noexcept { return context_; }

    /// @brief Await and read the next incoming message
    ///
    /// On end-of-input, `Finish` is called automatically.
    ///
    /// @param response where to put response on success
    /// @returns `true` on success, `false` on end-of-input, task cancellation,
    ///              or if the stream is already closed for reads
    /// @throws ugrpc::client::RpcError on an RPC error
    [[nodiscard]] bool Read(Response& response);

    /// @brief Return future to read next incoming result
    ///
    /// @param response where to put response on success
    /// @return StreamReadFuture future
    /// @throws ugrpc::client::RpcError on an RPC error
    /// @throws ugrpc::client::RpcError if the stream is already closed for reads
    StreamReadFuture ReadAsync(Response& response);

    /// @brief Write the next outgoing message
    ///
    /// RPC will be performed immediately. No references to `request` are
    /// saved, so it can be deallocated right after the call.
    ///
    /// @param request the next message to write
    /// @return true if the data is going to the wire; false if the write
    ///         operation failed (including due to task cancellation,
    //          or if the stream is already closed for writes),
    ///         in which case no more writes will be accepted,
    ///         but Read may still have some data and status code available
    [[nodiscard]] bool Write(const Request& request);

    /// @brief Write the next outgoing message and check result
    ///
    /// `WriteAndCheck` doesn't store any references to `request`, so it can be
    /// deallocated right after the call.
    ///
    /// `WriteAndCheck` verifies result of the write and generates exception
    /// in case of issues.
    ///
    /// @param request the next message to write
    /// @throws ugrpc::client::RpcError on an RPC error
    /// @throws ugrpc::client::RpcCancelledError on task cancellation
    /// @throws ugrpc::client::RpcError if the stream is already closed for writes
    void WriteAndCheck(const Request& request);

    /// @brief Announce end-of-output to the server
    ///
    /// Should be called to notify the server and receive the final response(s).
    ///
    /// @return true if the data is going to the wire; false if the operation
    ///         failed (including if the stream is already closed for writes),
    ///         but Read may still have some data and status code available
    [[nodiscard]] bool WritesDone();

private:
    StreamingCallState state_;
    CallContext context_;
    RawReaderWriter<Request, Response> stream_;
};

template <typename Response>
template <typename Stub, typename Request>
InputStream<Response>::InputStream(
    CallParams&& params,
    PrepareServerStreamingCall<Stub, Request, Response> prepare_async_method,
    const Request& request
)
    : state_{std::move(params), CallKind::kInputStream}, context_{utils::impl::InternalTag{}, state_} {
    RunMiddlewarePipeline(state_, StartCallHooks(ToBaseMessage(&request)));

    // NOLINTNEXTLINE(cppcoreguidelines-prefer-member-initializer)
    stream_ = impl::PrepareCall(
        prepare_async_method, state_.GetStub(), &state_.GetClientContext(), request, &state_.GetQueue()
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
        RunMiddlewarePipeline(state_, RecvMessageHooks(response));
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
    : state_{std::move(params), CallKind::kOutputStream}, context_{utils::impl::InternalTag{}, state_} {
    RunMiddlewarePipeline(state_, StartCallHooks());

    // 'response_' will be filled upon successful 'Finish' async call
    // NOLINTNEXTLINE(cppcoreguidelines-prefer-member-initializer)
    stream_ = impl::PrepareCall(
        prepare_async_method, state_.GetStub(), &state_.GetClientContext(), &response_, &state_.GetQueue()
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

    RunMiddlewarePipeline(state_, SendMessageHooks(request));

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

    RunMiddlewarePipeline(state_, SendMessageHooks(request));

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
    : state_{std::move(params), CallKind::kBidirectionalStream}, context_{utils::impl::InternalTag{}, state_} {
    RunMiddlewarePipeline(state_, StartCallHooks());

    // NOLINTNEXTLINE(cppcoreguidelines-prefer-member-initializer)
    stream_ = impl::PrepareCall(prepare_async_method, state_.GetStub(), &state_.GetClientContext(), &state_.GetQueue());
    impl::StartCall(*stream_, state_);
}

template <typename Request, typename Response>
BidirectionalStream<Request, Response>::~BidirectionalStream() {
    impl::FinishAbandoned(*stream_, state_);
}

template <typename Request, typename Response>
typename BidirectionalStream<Request, Response>::StreamReadFuture BidirectionalStream<Request, Response>::ReadAsync(
    Response& response
) {
    if (!IsReadAvailable(state_)) {
        // If the stream is already finished, we must exit immediately.
        // If not, even the middlewares may access something that is already dead.
        throw RpcError(state_.GetCallName(), "'ReadAsync' called on a finished call");
    }

    impl::ReadAsync(*stream_, response, state_);
    return StreamReadFuture{state_, *stream_, ToBaseMessage(&response)};
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
        RunMiddlewarePipeline(state_, SendMessageHooks(request));
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
        RunMiddlewarePipeline(state_, SendMessageHooks(request));
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
