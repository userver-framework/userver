#pragma once

/// @file userver/ugrpc/client/rpc.hpp
/// @brief Classes representing an outgoing RPC

#include <memory>
#include <string_view>
#include <utility>

#include <grpcpp/impl/codegen/proto_utils.h>

#include <userver/utils/assert.hpp>
#include <userver/utils/clang_format_workarounds.hpp>

#include <userver/ugrpc/client/exceptions.hpp>
#include <userver/ugrpc/client/impl/async_methods.hpp>
#include <userver/ugrpc/impl/deadline_timepoint.hpp>
#include <userver/ugrpc/impl/statistics_scope.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

/// @brief Controls a single request -> single response RPC
///
/// The RPC is cancelled on destruction unless `Finish` has been called.
template <typename Response>
class USERVER_NODISCARD UnaryCall final {
 public:
  /// @brief Await and read the response
  ///
  /// `Finish` must not be called multiple times for the same RPC.
  ///
  /// @returns the response on success
  /// @throws ugrpc::client::RpcError on an RPC error
  Response Finish();

  /// @returns the `ClientContext` used for this RPC
  const grpc::ClientContext& GetContext() const;

  /// For internal use only
  template <typename Stub, typename Request>
  UnaryCall(
      Stub& stub, grpc::CompletionQueue& queue,
      impl::RawResponseReaderPreparer<Stub, Request, Response> prepare_func,
      std::string_view call_name, std::unique_ptr<grpc::ClientContext> context,
      ugrpc::impl::MethodStatistics& statistics, const Request& req);

  UnaryCall(UnaryCall&&) noexcept = default;
  UnaryCall& operator=(UnaryCall&&) noexcept = default;
  ~UnaryCall();

 private:
  std::unique_ptr<grpc::ClientContext> context_;
  std::string_view call_name_;
  ugrpc::impl::RpcStatisticsScope statistics_;
  impl::RawResponseReader<Response> reader_;
  bool is_finished_{false};
};

/// @brief Controls a single request -> response stream RPC
///
/// This class is not thread-safe except for `GetContext`.
///
/// The RPC is cancelled on destruction unless the stream is closed (`Read` has
/// returned `false`).
///
/// If any method throws, further methods must not be called on the same stream,
/// except for `GetContext`.
template <typename Response>
class USERVER_NODISCARD InputStream final {
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
  const grpc::ClientContext& GetContext() const;

  /// For internal use only
  template <typename Stub, typename Request>
  InputStream(Stub& stub, grpc::CompletionQueue& queue,
              impl::RawReaderPreparer<Stub, Request, Response> prepare_func,
              std::string_view call_name,
              std::unique_ptr<grpc::ClientContext> context,
              ugrpc::impl::MethodStatistics& statistics, const Request& req);

  InputStream(InputStream&&) noexcept = default;
  InputStream& operator=(InputStream&&) noexcept = default;
  ~InputStream();

 private:
  std::unique_ptr<grpc::ClientContext> context_;
  std::string_view call_name_;
  ugrpc::impl::RpcStatisticsScope statistics_;
  impl::RawReader<Response> stream_;
  bool is_finished_{false};
};

/// @brief Controls a request stream -> single response RPC
///
/// This class is not thread-safe except for `GetContext`.
///
/// The RPC is cancelled on destruction unless `Finish` has been called.
///
/// If any method throws, further methods must not be called on the same stream,
/// except for `GetContext`.
template <typename Request, typename Response>
class USERVER_NODISCARD OutputStream final {
 public:
  /// @brief Write the next outgoing message
  ///
  /// `Write` doesn't store any references to `request`, so it can be
  /// deallocated right after the call.
  ///
  /// @param request the next message to write
  /// @throws ugrpc::client::RpcError on an RPC error
  void Write(const Request& request);

  /// @brief Complete the RPC successfully
  ///
  /// Should be called once all the data is written. The server will then
  /// send a single `Response`.
  ///
  /// `Finish` must not be called multiple times.
  ///
  /// @returns the single `Response` received after finishing the writes
  /// @throws ugrpc::client::RpcError on an RPC error
  Response Finish();

  /// @returns the `ClientContext` used for this RPC
  const grpc::ClientContext& GetContext() const;

  /// For internal use only
  template <typename Stub>
  OutputStream(Stub& stub, grpc::CompletionQueue& queue,
               impl::RawWriterPreparer<Stub, Request, Response> prepare_func,
               std::string_view call_name,
               std::unique_ptr<grpc::ClientContext> context,
               ugrpc::impl::MethodStatistics& statistics);

  OutputStream(OutputStream&&) noexcept = default;
  OutputStream& operator=(OutputStream&&) noexcept = default;
  ~OutputStream();

 private:
  std::unique_ptr<grpc::ClientContext> context_;
  std::string_view call_name_;
  ugrpc::impl::RpcStatisticsScope statistics_;
  std::unique_ptr<Response> final_response_;
  impl::RawWriter<Request> stream_;
  bool is_finished_{false};
};

/// @brief Controls a request stream -> response stream RPC
///
/// This class is not thread-safe except for `GetContext`.
///
/// The RPC is cancelled on destruction unless the stream is closed (`Read` has
/// returned `false`).
///
/// If any method throws, further methods must not be called on the same stream,
/// except for `GetContext`.
template <typename Request, typename Response>
class USERVER_NODISCARD BidirectionalStream final {
 public:
  /// @brief Await and read the next incoming message
  ///
  /// On end-of-input, `Finish` is called automatically. If by that time
  /// `WritesDone` has not been called, `UnexpectedEndOfInput` is thrown.
  ///
  /// @param response where to put response on success
  /// @returns `true` on success, `false` on end-of-input
  /// @throws ugrpc::client::RpcError on an RPC error
  /// @throws UnexpectedEndOfInput on end-of-input, if `WritesDone` was not
  /// called
  [[nodiscard]] bool Read(Response& response);

  /// @brief Write the next outgoing message
  ///
  /// RPC will be performed immediately. No references to `request` are
  /// saved, so it can be deallocated right after the call.
  ///
  /// @param request the next message to write
  /// @throws ugrpc::client::RpcError on an RPC error
  void Write(const Request& request);

  /// @brief Announce end-of-output to the server
  ///
  /// Should be called to notify the server and receive the final response(s).
  ///
  /// @throws ugrpc::client::RpcError on an RPC error
  void WritesDone();

  /// @returns the `ClientContext` used for this RPC
  const grpc::ClientContext& GetContext() const;

  /// For internal use only
  template <typename Stub>
  BidirectionalStream(
      Stub& stub, grpc::CompletionQueue& queue,
      impl::RawReaderWriterPreparer<Stub, Request, Response> prepare_func,
      std::string_view call_name, std::unique_ptr<grpc::ClientContext> context,
      ugrpc::impl::MethodStatistics& statistics);

  BidirectionalStream(BidirectionalStream&&) noexcept = default;
  BidirectionalStream& operator=(BidirectionalStream&&) noexcept = default;
  ~BidirectionalStream();

 private:
  enum class State { kOpen, kWritesDone, kFinished };

  std::unique_ptr<grpc::ClientContext> context_;
  std::string_view call_name_;
  ugrpc::impl::RpcStatisticsScope statistics_;
  impl::RawReaderWriter<Request, Response> stream_;
  State state_{State::kOpen};
};

// ========================== Implementation follows ==========================

template <typename Response>
template <typename Stub, typename Request>
UnaryCall<Response>::UnaryCall(
    Stub& stub, grpc::CompletionQueue& queue,
    impl::RawResponseReaderPreparer<Stub, Request, Response> prepare_func,
    std::string_view call_name, std::unique_ptr<grpc::ClientContext> context,
    ugrpc::impl::MethodStatistics& statistics, const Request& req)
    : context_(std::move(context)),
      call_name_(call_name),
      statistics_(statistics),
      reader_((stub.*prepare_func)(context_.get(), req, &queue)) {
  UASSERT(context_);
  reader_->StartCall();
}

template <typename Response>
UnaryCall<Response>::~UnaryCall() {
  if (context_ && !is_finished_) context_->TryCancel();
}

template <typename Response>
const grpc::ClientContext& UnaryCall<Response>::GetContext() const {
  UASSERT(context_);
  return *context_;
}

template <typename Response>
Response UnaryCall<Response>::Finish() {
  UASSERT(context_);
  UINVARIANT(!is_finished_, "'Finish' called on a finished call");
  is_finished_ = true;

  Response response;
  grpc::Status status;
  impl::FinishUnary(*reader_, response, status, call_name_, statistics_);

  return response;
}

template <typename Response>
template <typename Stub, typename Request>
InputStream<Response>::InputStream(
    Stub& stub, grpc::CompletionQueue& queue,
    impl::RawReaderPreparer<Stub, Request, Response> prepare_func,
    std::string_view call_name, std::unique_ptr<grpc::ClientContext> context,
    ugrpc::impl::MethodStatistics& statistics, const Request& req)
    : context_(std::move(context)),
      call_name_(call_name),
      statistics_(statistics),
      stream_((stub.*prepare_func)(context_.get(), req, &queue)) {
  UASSERT(context_);
  impl::StartCall(*stream_, call_name_, statistics_);
}

template <typename Response>
InputStream<Response>::~InputStream() {
  if (context_ && !is_finished_) context_->TryCancel();
}

template <typename Response>
const grpc::ClientContext& InputStream<Response>::GetContext() const {
  UASSERT(context_);
  return *context_;
}

template <typename Response>
bool InputStream<Response>::Read(Response& response) {
  UASSERT(context_);
  UINVARIANT(!is_finished_, "'Read' called on a closed stream");

  if (impl::Read(*stream_, response)) {
    return true;
  } else {
    is_finished_ = true;
    impl::Finish(*stream_, call_name_, statistics_);
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
    : context_(std::move(context)),
      call_name_(call_name),
      statistics_(statistics),
      final_response_(std::make_unique<Response>()),
      // 'final_response_' will be filled upon successful 'Finish' async call
      stream_(
          (stub.*prepare_func)(context_.get(), final_response_.get(), &queue)) {
  UASSERT(context_);
  impl::StartCall(*stream_, call_name, statistics_);
}

template <typename Request, typename Response>
OutputStream<Request, Response>::~OutputStream() {
  if (context_ && !is_finished_) context_->TryCancel();
}

template <typename Request, typename Response>
const grpc::ClientContext& OutputStream<Request, Response>::GetContext() const {
  UASSERT(context_);
  return *context_;
}

template <typename Request, typename Response>
void OutputStream<Request, Response>::Write(const Request& request) {
  UASSERT(context_);
  UINVARIANT(!is_finished_, "'Write' called on a finished stream");

  // It is safe to always buffer writes in a non-interactive stream
  const auto write_options = grpc::WriteOptions().set_buffer_hint();

  if (!impl::Write(*stream_, request, write_options)) {
    is_finished_ = true;
    impl::Finish(*stream_, call_name_, statistics_);

    // The server has somehow not returned error details on 'Finish',
    // but we know better that 'Write' has failed
    statistics_.OnNetworkError();
    throw RpcInterruptedError(call_name_, "Write");
  }
}

template <typename Request, typename Response>
Response OutputStream<Request, Response>::Finish() {
  UASSERT(context_);
  UINVARIANT(!is_finished_, "'Finish' called on a finished stream");
  is_finished_ = true;

  // gRPC does not implicitly call `WritesDone` in `Finish`,
  // contrary to the documentation
  const bool writes_done_success = impl::WritesDone(*stream_);

  impl::Finish(*stream_, call_name_, statistics_);

  if (!writes_done_success) {
    // The server has somehow not returned error details on 'Finish',
    // but we know better that 'WritesDone' has failed
    statistics_.OnNetworkError();
    throw RpcInterruptedError(call_name_, "WritesDone");
  }

  return std::move(*final_response_);
}

template <typename Request, typename Response>
template <typename Stub>
BidirectionalStream<Request, Response>::BidirectionalStream(
    Stub& stub, grpc::CompletionQueue& queue,
    impl::RawReaderWriterPreparer<Stub, Request, Response> prepare_func,
    std::string_view call_name, std::unique_ptr<grpc::ClientContext> context,
    ugrpc::impl::MethodStatistics& statistics)
    : context_(std::move(context)),
      call_name_(call_name),
      statistics_(statistics),
      stream_((stub.*prepare_func)(context_.get(), &queue)) {
  UASSERT(context_);
  impl::StartCall(*stream_, call_name, statistics_);
}

template <typename Request, typename Response>
BidirectionalStream<Request, Response>::~BidirectionalStream() {
  if (context_ && state_ != State::kFinished) context_->TryCancel();
}

template <typename Request, typename Response>
const grpc::ClientContext& BidirectionalStream<Request, Response>::GetContext()
    const {
  UASSERT(context_);
  return *context_;
}

template <typename Request, typename Response>
bool BidirectionalStream<Request, Response>::Read(Response& response) {
  UASSERT(context_);
  UINVARIANT(state_ != State::kFinished, "'Read' called on a finished stream");

  if (impl::Read(*stream_, response)) {
    return true;
  } else {
    state_ = State::kFinished;
    impl::Finish(*stream_, call_name_, statistics_);
    return false;
  }
}

template <typename Request, typename Response>
void BidirectionalStream<Request, Response>::Write(const Request& request) {
  UASSERT(context_);
  UINVARIANT(state_ == State::kOpen,
             "'Write' called on a stream that is closed for writes");

  // Don't buffer writes, optimize for ping-pong-style interaction
  grpc::WriteOptions write_options{};

  if (!impl::Write(*stream_, request, write_options)) {
    state_ = State::kFinished;
    impl::Finish(*stream_, call_name_, statistics_);

    // The server has somehow not returned error details on 'Finish',
    // but we know better that 'Write' has failed
    statistics_.OnNetworkError();
    throw RpcInterruptedError(call_name_, "Write");
  }
}

template <typename Request, typename Response>
void BidirectionalStream<Request, Response>::WritesDone() {
  UASSERT(context_);
  UINVARIANT(state_ == State::kOpen,
             "'WritesDone' called twice on the same stream");
  state_ = State::kWritesDone;

  if (!impl::WritesDone(*stream_)) {
    state_ = State::kFinished;
    impl::Finish(*stream_, call_name_, statistics_);

    // The server has somehow not returned error details on 'Finish',
    // but we know better that 'Write' has failed
    statistics_.OnNetworkError();
    throw RpcInterruptedError(call_name_, "WritesDone");
  }
}

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
