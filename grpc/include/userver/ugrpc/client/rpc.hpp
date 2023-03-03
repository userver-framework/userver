#pragma once

/// @file userver/ugrpc/client/rpc.hpp
/// @brief Classes representing an outgoing RPC

#include <memory>
#include <string_view>
#include <utility>

#include <grpcpp/impl/codegen/proto_utils.h>

#include <userver/utils/assert.hpp>

#include <userver/ugrpc/client/exceptions.hpp>
#include <userver/ugrpc/client/impl/async_methods.hpp>
#include <userver/ugrpc/impl/deadline_timepoint.hpp>
#include <userver/ugrpc/impl/statistics_scope.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

/// @brief UnaryFuture for waiting a single response RPC
class [[nodiscard]] UnaryFuture {
 public:
  /// @cond
  explicit UnaryFuture(impl::RpcData& data) noexcept;
  /// @endcond

  UnaryFuture(UnaryFuture&&) noexcept = default;
  UnaryFuture& operator=(UnaryFuture&&) noexcept;

  ~UnaryFuture() noexcept;

  /// @brief Await response
  ///
  /// Upon completion result is available in `response` when initiating the
  /// asynchronous operation, e.g. FinishAsync.
  ///
  /// `Get` should not be called multiple times for the same UnaryFuture.
  ///
  /// @throws ugrpc::client::RpcError on an RPC error
  void Get();

  /// @brief Checks if the asynchronous call has completed
  ///        Note, that once user gets result, IsReady should not be called
  /// @return true if result ready
  [[nodiscard]] bool IsReady() const noexcept;

 private:
  impl::FutureImpl impl_;
};

/// @brief StreamReadFuture for waiting a single read response from stream
template <typename RPC>
class [[nodiscard]] StreamReadFuture {
 public:
  /// @cond
  explicit StreamReadFuture(impl::RpcData& data,
                            typename RPC::RawStream& stream) noexcept;
  /// @endcond

  StreamReadFuture(StreamReadFuture&& other) noexcept = default;
  StreamReadFuture& operator=(StreamReadFuture&& other) noexcept;

  ~StreamReadFuture() noexcept;

  /// @brief Await response
  ///
  /// Upon completion the result is available in `response` that was
  /// specified when initiating the asynchronous read
  ///
  /// `Get` should not be called multiple times for the same StreamReadFuture.
  ///
  /// @throws ugrpc::client::RpcError on an RPC error
  bool Get();

  /// @brief Checks if the asynchronous call has completed
  ///        Note, that once user gets result, IsReady should not be called
  /// @return true if result ready
  [[nodiscard]] bool IsReady() const noexcept;

 private:
  impl::FutureImpl impl_;
  typename RPC::RawStream* stream_;
};

/// @brief Controls a single request -> single response RPC
///
/// This class is not thread-safe except for `GetContext`.
///
/// The RPC is cancelled on destruction unless `Finish` or `FinishAsync`. In
/// that case the connection is not closed (it will be reused for new RPCs), and
/// the server receives `RpcInterruptedError` immediately.
template <typename Response>
class [[nodiscard]] UnaryCall final {
 public:
  /// @brief Await and read the response
  ///
  /// `Finish` should not be called multiple times for the same RPC.
  ///
  /// The connection is not closed, it will be reused for new RPCs.
  ///
  /// @returns the response on success
  /// @throws ugrpc::client::RpcError on an RPC error
  Response Finish();

  /// @brief Asynchronously finish the call
  ///
  /// `FinishAsync` should not be called multiple times for the same RPC.
  ///
  /// `Finish` and `FinishAsync` should not be called together for the same RPC.
  ///
  /// @returns the future for the single response
  UnaryFuture FinishAsync(Response& response);

  /// @returns the `ClientContext` used for this RPC
  grpc::ClientContext& GetContext();

  /// @cond
  // For internal use only
  template <typename Stub, typename Request>
  UnaryCall(
      Stub& stub, grpc::CompletionQueue& queue,
      impl::RawResponseReaderPreparer<Stub, Request, Response> prepare_func,
      std::string_view call_name, std::unique_ptr<grpc::ClientContext> context,
      ugrpc::impl::MethodStatistics& statistics, const Request& req);
  /// @endcond

  UnaryCall(UnaryCall&&) noexcept = default;
  UnaryCall& operator=(UnaryCall&&) noexcept = default;
  ~UnaryCall() = default;

 private:
  std::unique_ptr<impl::RpcData> data_;
  impl::RawResponseReader<Response> reader_;
};

/// @brief Controls a single request -> response stream RPC
///
/// This class is not thread-safe except for `GetContext`.
///
/// The RPC is cancelled on destruction unless the stream is closed (`Read` has
/// returned `false`). In that case the connection is not closed (it will be
/// reused for new RPCs), and the server receives `RpcInterruptedError`
/// immediately. gRPC provides no way to early-close a server-streaming RPC
/// gracefully.
///
/// If any method throws, further methods must not be called on the same stream,
/// except for `GetContext`.
template <typename Response>
class [[nodiscard]] InputStream final {
 public:
  /// @brief Await and read the next incoming message
  ///
  /// On end-of-input, `Finish` is called automatically.
  ///
  /// @param response where to put response on success
  /// @returns `true` on success, `false` on end-of-input
  /// @throws ugrpc::client::RpcError on an RPC error
  [[nodiscard]] bool Read(Response& response);

  /// @returns the `ClientContext` used for this RPC
  grpc::ClientContext& GetContext();

  /// @cond
  // For internal use only
  using RawStream = grpc::ClientAsyncReader<Response>;

  template <typename Stub, typename Request>
  InputStream(Stub& stub, grpc::CompletionQueue& queue,
              impl::RawReaderPreparer<Stub, Request, Response> prepare_func,
              std::string_view call_name,
              std::unique_ptr<grpc::ClientContext> context,
              ugrpc::impl::MethodStatistics& statistics, const Request& req);
  /// @endcond

  InputStream(InputStream&&) noexcept = default;
  InputStream& operator=(InputStream&&) noexcept = default;
  ~InputStream() = default;

 private:
  std::unique_ptr<impl::RpcData> data_;
  impl::RawReader<Response> stream_;
};

/// @brief Controls a request stream -> single response RPC
///
/// This class is not thread-safe except for `GetContext`.
///
/// The RPC is cancelled on destruction unless `Finish` has been called. In that
/// case the connection is not closed (it will be reused for new RPCs), and the
/// server receives `RpcInterruptedError` immediately.
///
/// If any method throws, further methods must not be called on the same stream,
/// except for `GetContext`.
template <typename Request, typename Response>
class [[nodiscard]] OutputStream final {
 public:
  /// @brief Write the next outgoing message
  ///
  /// `Write` doesn't store any references to `request`, so it can be
  /// deallocated right after the call.
  ///
  /// @param request the next message to write
  /// @return true if the data is going to the wire; false if the write
  ///         operation failed, in which case no more writes will be accepted,
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
  Response Finish();

  /// @returns the `ClientContext` used for this RPC
  grpc::ClientContext& GetContext();

  /// @cond
  // For internal use only
  using RawStream = grpc::ClientAsyncWriter<Request>;

  template <typename Stub>
  OutputStream(Stub& stub, grpc::CompletionQueue& queue,
               impl::RawWriterPreparer<Stub, Request, Response> prepare_func,
               std::string_view call_name,
               std::unique_ptr<grpc::ClientContext> context,
               ugrpc::impl::MethodStatistics& statistics);
  /// @endcond

  OutputStream(OutputStream&&) noexcept = default;
  OutputStream& operator=(OutputStream&&) noexcept = default;
  ~OutputStream() = default;

 private:
  std::unique_ptr<impl::RpcData> data_;
  std::unique_ptr<Response> final_response_;
  impl::RawWriter<Request> stream_;
};

/// @brief Controls a request stream -> response stream RPC
///
/// This class allows the following concurrent calls:
///   - `GetContext`
///   - Concurrent call of one of (`Read`, `ReadAsync`) with one of (`Write`,
///     `WritesDone`)
/// `WriteAndCheck` is not thread-safe
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
template <typename Request, typename Response>
class [[nodiscard]] BidirectionalStream final {
 public:
  /// @brief Await and read the next incoming message
  ///
  /// On end-of-input, `Finish` is called automatically.
  ///
  /// @param response where to put response on success
  /// @returns `true` on success, `false` on end-of-input
  /// @throws ugrpc::client::RpcError on an RPC error
  [[nodiscard]] bool Read(Response& response);

  /// @brief Return future to read next incoming result
  ///
  /// @param response where to put response on success
  /// @return StreamReadFuture future
  /// @throws ugrpc::client::RpcError on an RPC error
  StreamReadFuture<BidirectionalStream> ReadAsync(Response& response) noexcept;

  /// @brief Write the next outgoing message
  ///
  /// RPC will be performed immediately. No references to `request` are
  /// saved, so it can be deallocated right after the call.
  ///
  /// @param request the next message to write
  /// @return true if the data is going to the wire; false if the write
  ///         operation failed, in which case no more writes will be accepted,
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
  void WriteAndCheck(const Request& request);

  /// @brief Announce end-of-output to the server
  ///
  /// Should be called to notify the server and receive the final response(s).
  ///
  /// @return true if the data is going to the wire; false if the operation
  ///         failed, but Read may still have some data and status code
  ///         available
  [[nodiscard]] bool WritesDone();

  /// @returns the `ClientContext` used for this RPC
  grpc::ClientContext& GetContext();

  /// @cond
  // For internal use only
  using RawStream = grpc::ClientAsyncReaderWriter<Request, Response>;

  template <typename Stub>
  BidirectionalStream(
      Stub& stub, grpc::CompletionQueue& queue,
      impl::RawReaderWriterPreparer<Stub, Request, Response> prepare_func,
      std::string_view call_name, std::unique_ptr<grpc::ClientContext> context,
      ugrpc::impl::MethodStatistics& statistics);
  /// @endcond

  BidirectionalStream(BidirectionalStream&&) noexcept = default;
  BidirectionalStream& operator=(BidirectionalStream&&) noexcept = default;
  ~BidirectionalStream() = default;

 private:
  std::unique_ptr<impl::RpcData> data_;
  impl::RawReaderWriter<Request, Response> stream_;
};

// ========================== Implementation follows ==========================

template <typename RPC>
StreamReadFuture<RPC>::StreamReadFuture(
    impl::RpcData& data, typename RPC::RawStream& stream) noexcept
    : impl_(data), stream_(&stream) {}

template <typename RPC>
StreamReadFuture<RPC>::~StreamReadFuture() noexcept {
  if (auto* const data = impl_.GetData()) {
    impl::RpcData::AsyncMethodInvocationGuard guard(*data);
    if (!data->GetAsyncMethodInvocation().Wait()) {
      impl::Finish(*stream_, *data, false);
    }
  }
}

template <typename RPC>
StreamReadFuture<RPC>& StreamReadFuture<RPC>::operator=(
    StreamReadFuture<RPC>&& other) noexcept {
  if (this == &other) return *this;
  [[maybe_unused]] auto for_destruction = std::move(*this);
  impl_ = std::move(other.impl_);
  stream_ = other.stream_;
  return *this;
}

template <typename RPC>
bool StreamReadFuture<RPC>::Get() {
  auto* const data = impl_.GetData();
  UINVARIANT(data, "'Get' must be called only once");
  impl::RpcData::AsyncMethodInvocationGuard guard(*data);
  impl_.ClearData();
  auto result = data->GetAsyncMethodInvocation().Wait();
  if (!result) {
    // Finish can only be called once all the data is read, otherwise the
    // underlying gRPC driver hangs.
    impl::Finish(*stream_, *data, true);
  }
  return result;
}

template <typename RPC>
bool StreamReadFuture<RPC>::IsReady() const noexcept {
  return impl_.IsReady();
}

template <typename Response>
template <typename Stub, typename Request>
UnaryCall<Response>::UnaryCall(
    Stub& stub, grpc::CompletionQueue& queue,
    impl::RawResponseReaderPreparer<Stub, Request, Response> prepare_func,
    std::string_view call_name, std::unique_ptr<grpc::ClientContext> context,
    ugrpc::impl::MethodStatistics& statistics, const Request& req)
    : data_(std::make_unique<impl::RpcData>(std::move(context), call_name,
                                            statistics)),
      reader_((stub.*prepare_func)(&data_->GetContext(), req, &queue)) {
  reader_->StartCall();
  data_->SetWritesFinished();
}

template <typename Response>
grpc::ClientContext& UnaryCall<Response>::GetContext() {
  return data_->GetContext();
}

template <typename Response>
Response UnaryCall<Response>::Finish() {
  Response response;
  UnaryFuture future = FinishAsync(response);
  future.Get();
  return response;
}

template <typename Response>
UnaryFuture UnaryCall<Response>::FinishAsync(Response& response) {
  PrepareFinish(*data_);
  data_->EmplaceAsyncMethodInvocation();
  auto& finish = data_->GetAsyncMethodInvocation();
  auto& status = data_->GetStatus();
  reader_->Finish(&response, &status, finish.GetTag());
  return UnaryFuture{*data_};
}

template <typename Response>
template <typename Stub, typename Request>
InputStream<Response>::InputStream(
    Stub& stub, grpc::CompletionQueue& queue,
    impl::RawReaderPreparer<Stub, Request, Response> prepare_func,
    std::string_view call_name, std::unique_ptr<grpc::ClientContext> context,
    ugrpc::impl::MethodStatistics& statistics, const Request& req)
    : data_(std::make_unique<impl::RpcData>(std::move(context), call_name,
                                            statistics)),
      stream_((stub.*prepare_func)(&data_->GetContext(), req, &queue)) {
  impl::StartCall(*stream_, *data_);
  data_->SetWritesFinished();
}

template <typename Response>
grpc::ClientContext& InputStream<Response>::GetContext() {
  return data_->GetContext();
}

template <typename Response>
bool InputStream<Response>::Read(Response& response) {
  if (impl::Read(*stream_, response, *data_)) {
    return true;
  } else {
    // Finish can only be called once all the data is read, otherwise the
    // underlying gRPC driver hangs.
    impl::Finish(*stream_, *data_, true);
    return false;
  }
}

template <typename Request, typename Response>
template <typename Stub>
OutputStream<Request, Response>::OutputStream(
    Stub& stub, grpc::CompletionQueue& queue,
    impl::RawWriterPreparer<Stub, Request, Response> prepare_func,
    std::string_view call_name, std::unique_ptr<grpc::ClientContext> context,
    ugrpc::impl::MethodStatistics& statistics)
    : data_(std::make_unique<impl::RpcData>(std::move(context), call_name,
                                            statistics)),
      final_response_(std::make_unique<Response>()),
      // 'final_response_' will be filled upon successful 'Finish' async call
      stream_((stub.*prepare_func)(&data_->GetContext(), final_response_.get(),
                                   &queue)) {
  impl::StartCall(*stream_, *data_);
}

template <typename Request, typename Response>
grpc::ClientContext& OutputStream<Request, Response>::GetContext() {
  return data_->GetContext();
}

template <typename Request, typename Response>
bool OutputStream<Request, Response>::Write(const Request& request) {
  // Don't buffer writes, otherwise in an event subscription scenario, events
  // may never actually be delivered
  grpc::WriteOptions write_options{};

  return impl::Write(*stream_, request, write_options, *data_);
}

template <typename Request, typename Response>
void OutputStream<Request, Response>::WriteAndCheck(const Request& request) {
  // Don't buffer writes, otherwise in an event subscription scenario, events
  // may never actually be delivered
  grpc::WriteOptions write_options{};

  if (!impl::Write(*stream_, request, write_options, *data_)) {
    impl::Finish(*stream_, *data_, true);
  }
}

template <typename Request, typename Response>
Response OutputStream<Request, Response>::Finish() {
  // gRPC does not implicitly call `WritesDone` in `Finish`,
  // contrary to the documentation
  if (!data_->AreWritesFinished()) {
    impl::WritesDone(*stream_, *data_);
  }

  impl::Finish(*stream_, *data_, true);

  return std::move(*final_response_);
}

template <typename Request, typename Response>
template <typename Stub>
BidirectionalStream<Request, Response>::BidirectionalStream(
    Stub& stub, grpc::CompletionQueue& queue,
    impl::RawReaderWriterPreparer<Stub, Request, Response> prepare_func,
    std::string_view call_name, std::unique_ptr<grpc::ClientContext> context,
    ugrpc::impl::MethodStatistics& statistics)
    : data_(std::make_unique<impl::RpcData>(std::move(context), call_name,
                                            statistics)),
      stream_((stub.*prepare_func)(&data_->GetContext(), &queue)) {
  impl::StartCall(*stream_, *data_);
}

template <typename Request, typename Response>
grpc::ClientContext& BidirectionalStream<Request, Response>::GetContext() {
  return data_->GetContext();
}

template <typename Request, typename Response>
StreamReadFuture<BidirectionalStream<Request, Response>>
BidirectionalStream<Request, Response>::ReadAsync(Response& response) noexcept {
  impl::ReadAsync(*stream_, response, *data_);
  return StreamReadFuture<BidirectionalStream<Request, Response>>{*data_,
                                                                  *stream_};
}

template <typename Request, typename Response>
bool BidirectionalStream<Request, Response>::Read(Response& response) {
  auto future = ReadAsync(response);
  return future.Get();
}

template <typename Request, typename Response>
bool BidirectionalStream<Request, Response>::Write(const Request& request) {
  // Don't buffer writes, optimize for ping-pong-style interaction
  grpc::WriteOptions write_options{};

  return impl::Write(*stream_, request, write_options, *data_);
}

template <typename Request, typename Response>
void BidirectionalStream<Request, Response>::WriteAndCheck(
    const Request& request) {
  // Don't buffer writes, optimize for ping-pong-style interaction
  grpc::WriteOptions write_options{};

  impl::WriteAndCheck(*stream_, request, write_options, *data_);
}

template <typename Request, typename Response>
bool BidirectionalStream<Request, Response>::WritesDone() {
  return impl::WritesDone(*stream_, *data_);
}

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
