#pragma once

/// @file userver/ugrpc/client/stream.hpp
/// @brief Client streaming interfaces

#include <memory>

#include <userver/ugrpc/client/impl/rpc.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

/// @brief Client-side interface for server-streaming
///
/// This class is not thread-safe except for `GetContext`.
///
/// The RPC is cancelled on destruction unless the stream is closed (`Read` has
/// returned `false`). In that case the connection is not closed (it will be
/// reused for new RPCs), and the server receives `RpcInterruptedError`
/// immediately. gRPC provides no way to early-close a server-streaming RPC
/// gracefully.
template <class Response>
class [[nodiscard]] Reader final {
public:
    /// @cond
    // For internal use only
    template <typename Stub, typename Request>
    Reader(
        impl::CallParams&& params,
        impl::PrepareServerStreamingCall<Stub, Request, Response> prepare_async_method,
        const Request& request
    )
        : stream_(
              std::make_unique<impl::InputStream<Response>>(std::move(params), std::move(prepare_async_method), request)
          ) {}
    /// @endcond

    Reader(Reader&&) noexcept = default;
    Reader& operator=(Reader&&) noexcept = default;

    /// @brief Await and read the next incoming message
    ///
    /// On end-of-input, `Finish` is called automatically.
    ///
    /// @param response where to put response on success
    /// @returns `true` on success, `false` on end-of-input, task cancellation,
    //           or if the stream is already closed for reads
    /// @throws ugrpc::client::RpcError on an RPC error
    [[nodiscard]] bool Read(Response& response) { return stream_->Read(response); }

    /// @brief Get call context, useful e.g. for accessing metadata.
    CallContext& GetContext() { return stream_->GetContext(); }
    /// @overload
    const CallContext& GetContext() const { return stream_->GetContext(); }

private:
    std::unique_ptr<impl::InputStream<Response>> stream_;
};

/// @brief Client-side interface for client-streaming
///
/// This class is not thread-safe except for `GetContext`.
///
/// The RPC is cancelled on destruction unless `Finish` has been called. In that
/// case the connection is not closed (it will be reused for new RPCs), and the
/// server receives `RpcInterruptedError` immediately.
template <typename Request, typename Response>
class [[nodiscard]] Writer final {
public:
    /// @cond
    // For internal use only
    template <typename Stub>
    Writer(impl::CallParams&& params, impl::PrepareClientStreamingCall<Stub, Request, Response> prepare_async_method)
        : stream_(std::make_unique<impl::OutputStream<Request, Response>>(
              std::move(params),
              std::move(prepare_async_method)
          )) {}
    /// @endcond

    Writer(Writer&&) noexcept = default;
    Writer& operator=(Writer&&) noexcept = default;

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
    [[nodiscard]] bool Write(const Request& request) { return stream_->Write(request); }

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
    void WriteAndCheck(const Request& request) { stream_->WriteAndCheck(request); }

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
    Response Finish() { return stream_->Finish(); }

    /// @brief Get call context, useful e.g. for accessing metadata.
    CallContext& GetContext() { return stream_->GetContext(); }
    /// @overload
    const CallContext& GetContext() const { return stream_->GetContext(); }

private:
    std::unique_ptr<impl::OutputStream<Request, Response>> stream_;
};

/// @brief Client-side interface for bi-directional streaming
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
class [[nodiscard]] ReaderWriter final {
public:
    using StreamReadFuture =
        ugrpc::client::StreamReadFuture<typename impl::BidirectionalStream<Request, Response>::RawStream>;

    /// @cond
    // For internal use only
    template <typename Stub>
    ReaderWriter(
        impl::CallParams&& params,
        impl::PrepareBidiStreamingCall<Stub, Request, Response> prepare_async_method
    )
        : stream_(std::make_unique<impl::BidirectionalStream<Request, Response>>(
              std::move(params),
              std::move(prepare_async_method)
          )) {}
    /// @endcond

    ReaderWriter(ReaderWriter&&) noexcept = default;
    ReaderWriter& operator=(ReaderWriter&&) noexcept = default;

    /// @brief Await and read the next incoming message
    ///
    /// On end-of-input, `Finish` is called automatically.
    ///
    /// @param response where to put response on success
    /// @returns `true` on success, `false` on end-of-input, task cancellation,
    ///              or if the stream is already closed for reads
    /// @throws ugrpc::client::RpcError on an RPC error
    [[nodiscard]] bool Read(Response& response) { return stream_->Read(response); }

    /// @brief Return future to read next incoming result
    ///
    /// @param response where to put response on success
    /// @return StreamReadFuture future
    /// @throws ugrpc::client::RpcError on an RPC error
    /// @throws ugrpc::client::RpcError if the stream is already closed for reads
    StreamReadFuture ReadAsync(Response& response) { return stream_->ReadAsync(response); }

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
    [[nodiscard]] bool Write(const Request& request) { return stream_->Write(request); }

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
    void WriteAndCheck(const Request& request) { stream_->WriteAndCheck(request); }

    /// @brief Announce end-of-output to the server
    ///
    /// Should be called to notify the server and receive the final response(s).
    ///
    /// @return true if the data is going to the wire; false if the operation
    ///         failed (including if the stream is already closed for writes),
    ///         but Read may still have some data and status code available
    [[nodiscard]] bool WritesDone() { return stream_->WritesDone(); }

    /// @brief Get call context, useful e.g. for accessing metadata.
    CallContext& GetContext() { return stream_->GetContext(); }
    /// @overload
    const CallContext& GetContext() const { return stream_->GetContext(); }

private:
    std::unique_ptr<impl::BidirectionalStream<Request, Response>> stream_;
};

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
