#pragma once

/// @file userver/ugrpc/server/rpc.hpp
/// @brief Classes representing an incoming RPC

#include <grpcpp/impl/codegen/proto_utils.h>
#include <grpcpp/server_context.h>

#include <userver/utils/assert.hpp>

#include <userver/ugrpc/impl/deadline_timepoint.hpp>
#include <userver/ugrpc/server/exceptions.hpp>
#include <userver/ugrpc/server/impl/async_methods.hpp>
#include <userver/ugrpc/server/impl/statistics_scope.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server {

/// @brief Controls a single request -> single response RPC
///
/// The RPC is cancelled on destruction unless `Finish` has been called.
template <typename Response>
class UnaryCall final {
 public:
  /// @brief Complete the RPC successfully
  ///
  /// `Finish` must not be called multiple times for the same RPC.
  ///
  /// @param response the single Response to send to the client
  /// @throws ugrpc::server::RpcError on an RPC error
  void Finish(const Response& response);

  /// @brief Complete the RPC with an error
  ///
  /// `Finish` must not be called multiple times for the same RPC.
  ///
  /// @param status error details
  /// @throws ugrpc::server::RpcError on an RPC error
  void FinishWithError(const grpc::Status& status);

  /// @returns the `ServerContext` used for this RPC
  /// @note Initial server metadata is not currently supported
  /// @note Trailing metadata, if any, must be set before the `Finish` call
  grpc::ServerContext& GetContext();

  /// For internal use only
  UnaryCall(grpc::ServerContext& context, std::string_view call_name,
            impl::RawResponseWriter<Response>& stream,
            impl::RpcStatisticsScope& statistics);

  UnaryCall(UnaryCall&&) = delete;
  UnaryCall& operator=(UnaryCall&&) = delete;
  ~UnaryCall();

 private:
  grpc::ServerContext& context_;
  const std::string_view call_name_;
  impl::RawResponseWriter<Response>& stream_;
  bool is_finished_{false};
  impl::RpcStatisticsScope& statistics_;
};

/// @brief Controls a request stream -> single response RPC
///
/// This class is not thread-safe except for `GetContext`.
///
/// The RPC is cancelled on destruction unless the stream has been finished.
///
/// If any method throws, further methods must not be called on the same stream,
/// except for `GetContext`.
template <typename Request, typename Response>
class InputStream final {
 public:
  /// @brief Await and read the next incoming message
  /// @param request where to put the request on success
  /// @returns `true` on success, `false` on end-of-input
  [[nodiscard]] bool Read(Request& request);

  /// @brief Complete the RPC successfully
  ///
  /// `Finish` must not be called multiple times for the same RPC.
  ///
  /// @param response the single Response to send to the client
  /// @throws ugrpc::server::RpcError on an RPC error
  void Finish(const Response& response);

  /// @brief Complete the RPC with an error
  ///
  /// `Finish` must not be called multiple times for the same RPC.
  ///
  /// @param status error details
  /// @throws ugrpc::server::RpcError on an RPC error
  void FinishWithError(const grpc::Status& status);

  /// @returns the `ServerContext` used for this RPC
  /// @note Initial server metadata is not currently supported
  /// @note Trailing metadata, if any, must be set before the `Finish` call
  grpc::ServerContext& GetContext();

  /// For internal use only
  InputStream(grpc::ServerContext& context, std::string_view call_name,
              impl::RawReader<Request, Response>& stream,
              impl::RpcStatisticsScope& statistics);

  InputStream(InputStream&&) = delete;
  InputStream& operator=(InputStream&&) = delete;
  ~InputStream();

 private:
  enum class State { kOpen, kReadsDone, kFinished };

  grpc::ServerContext& context_;
  const std::string_view call_name_;
  impl::RawReader<Request, Response>& stream_;
  State state_{State::kOpen};
  impl::RpcStatisticsScope& statistics_;
};

/// @brief Controls a single request -> response stream RPC
///
/// This class is not thread-safe except for `GetContext`.
///
/// The RPC is cancelled on destruction unless the stream has been finished.
///
/// If any method throws, further methods must not be called on the same stream,
/// except for `GetContext`.
template <typename Response>
class OutputStream final {
 public:
  /// @brief Write the next outgoing message
  /// @param response the next message to write
  /// @throws ugrpc::server::RpcError on an RPC error
  void Write(const Response& response);

  /// @brief Complete the RPC successfully
  ///
  /// `Finish` must not be called multiple times.
  ///
  /// @throws ugrpc::server::RpcError on an RPC error
  void Finish();

  /// @brief Complete the RPC with an error
  ///
  /// `Finish` must not be called multiple times.
  ///
  /// @param status error details
  /// @throws ugrpc::server::RpcError on an RPC error
  void FinishWithError(const grpc::Status& status);

  /// @brief Equivalent to `Write + Finish`
  ///
  /// This call saves one round-trip, compared to separate `Write` and `Finish`.
  ///
  /// `Finish` must not be called multiple times.
  ///
  /// @param response the final response message
  /// @throws ugrpc::server::RpcError on an RPC error
  void WriteAndFinish(const Response& response);

  /// @returns the `ServerContext` used for this RPC
  /// @note Initial server metadata is not currently supported
  /// @note Trailing metadata, if any, must be set before the `Finish` call
  grpc::ServerContext& GetContext();

  /// For internal use only
  OutputStream(grpc::ServerContext& context, std::string_view call_name,
               impl::RawWriter<Response>& stream,
               impl::RpcStatisticsScope& statistics);

  OutputStream(OutputStream&&) = delete;
  OutputStream& operator=(OutputStream&&) = delete;
  ~OutputStream();

 private:
  enum class State { kNew, kOpen, kFinished };

  grpc::ServerContext& context_;
  const std::string_view call_name_;
  impl::RawWriter<Response>& stream_;
  State state_{State::kNew};
  impl::RpcStatisticsScope& statistics_;
};

/// @brief Controls a request stream -> response stream RPC
///
/// This class is not thread-safe except for `GetContext`.
///
/// The RPC is cancelled on destruction unless the stream has been finished.
///
/// If any method throws, further methods must not be called on the same stream,
/// except for `GetContext`.
template <typename Request, typename Response>
class BidirectionalStream {
 public:
  /// @brief Await and read the next incoming message
  /// @param request where to put the request on success
  /// @returns `true` on success, `false` on end-of-input
  /// @throws ugrpc::server::RpcError on an RPC error
  [[nodiscard]] bool Read(Request& request);

  /// @brief Write the next outgoing message
  /// @param response the next message to write
  /// @throws ugrpc::server::RpcError on an RPC error
  void Write(const Response& response);

  /// @brief Complete the RPC successfully
  ///
  /// `Finish` must not be called multiple times.
  ///
  /// @throws ugrpc::server::RpcError on an RPC error
  void Finish();

  /// @brief Complete the RPC with an error
  ///
  /// `Finish` must not be called multiple times.
  ///
  /// @param status error details
  /// @throws ugrpc::server::RpcError on an RPC error
  void FinishWithError(const grpc::Status& status);

  /// @brief Equivalent to `Write + Finish`
  ///
  /// This call saves one round-trip, compared to separate `Write` and `Finish`.
  ///
  /// `Finish` must not be called multiple times.
  ///
  /// @param response the final response message
  /// @throws ugrpc::server::RpcError on an RPC error
  void WriteAndFinish(const Response& response);

  /// @returns the `ServerContext` used for this RPC
  /// @note Initial server metadata is not currently supported
  /// @note Trailing metadata, if any, must be set before the `Finish` call
  grpc::ServerContext& GetContext();

  /// For internal use only
  BidirectionalStream(grpc::ServerContext& context, std::string_view call_name,
                      impl::RawReaderWriter<Request, Response>& stream,
                      impl::RpcStatisticsScope& statistics);

  BidirectionalStream(const BidirectionalStream&) = delete;
  BidirectionalStream(BidirectionalStream&&) = delete;
  ~BidirectionalStream();

 private:
  enum class State { kOpen, kReadsDone, kFinished };

  grpc::ServerContext& context_;
  const std::string_view call_name_;
  impl::RawReaderWriter<Request, Response>& stream_;
  State state_{State::kOpen};
  impl::RpcStatisticsScope& statistics_;
};

// ========================== Implementation follows ==========================

template <typename Response>
UnaryCall<Response>::UnaryCall(grpc::ServerContext& context,
                               std::string_view call_name,
                               impl::RawResponseWriter<Response>& stream,
                               impl::RpcStatisticsScope& statistics)
    : context_(context),
      call_name_(call_name),
      stream_(stream),
      statistics_(statistics) {}

template <typename Response>
UnaryCall<Response>::~UnaryCall() {
  if (!is_finished_) impl::CancelWithError(stream_, call_name_);
}

template <typename Response>
grpc::ServerContext& UnaryCall<Response>::GetContext() {
  return context_;
}

template <typename Response>
void UnaryCall<Response>::Finish(const Response& response) {
  UINVARIANT(!is_finished_, "'Finish' called on a finished call");
  is_finished_ = true;
  impl::Finish(stream_, response, grpc::Status::OK, call_name_);
  statistics_.OnExplicitFinish(grpc::StatusCode::OK);
}

template <typename Response>
void UnaryCall<Response>::FinishWithError(const grpc::Status& status) {
  UINVARIANT(!is_finished_, "'FinishWithError' called on a finished call");
  is_finished_ = true;
  impl::FinishWithError(stream_, status, call_name_);
  statistics_.OnExplicitFinish(status.error_code());
}

template <typename Request, typename Response>
InputStream<Request, Response>::InputStream(
    grpc::ServerContext& context, std::string_view call_name,
    impl::RawReader<Request, Response>& stream,
    impl::RpcStatisticsScope& statistics)
    : context_(context),
      call_name_(call_name),
      stream_(stream),
      statistics_(statistics) {}

template <typename Request, typename Response>
InputStream<Request, Response>::~InputStream() {
  if (state_ != State::kFinished) impl::CancelWithError(stream_, call_name_);
}

template <typename Request, typename Response>
grpc::ServerContext& InputStream<Request, Response>::GetContext() {
  return context_;
}

template <typename Request, typename Response>
bool InputStream<Request, Response>::Read(Request& request) {
  UINVARIANT(state_ == State::kOpen,
             "'Read' called while the stream is half-closed for reads");
  if (impl::Read(stream_, request)) {
    return true;
  } else {
    state_ = State::kReadsDone;
    return false;
  }
}

template <typename Request, typename Response>
void InputStream<Request, Response>::Finish(const Response& response) {
  UINVARIANT(state_ != State::kFinished,
             "'Finish' called on a finished stream");
  state_ = State::kFinished;
  impl::Finish(stream_, response, grpc::Status::OK, call_name_);
  statistics_.OnExplicitFinish(grpc::StatusCode::OK);
}

template <typename Request, typename Response>
void InputStream<Request, Response>::FinishWithError(
    const grpc::Status& status) {
  UASSERT(!status.ok());
  UINVARIANT(state_ != State::kFinished,
             "'FinishWithError' called on a finished stream");
  state_ = State::kFinished;
  impl::FinishWithError(stream_, status, call_name_);
  statistics_.OnExplicitFinish(status.error_code());
}

template <typename Response>
OutputStream<Response>::OutputStream(grpc::ServerContext& context,
                                     std::string_view call_name,
                                     impl::RawWriter<Response>& stream,
                                     impl::RpcStatisticsScope& statistics)
    : context_(context),
      call_name_(call_name),
      stream_(stream),
      statistics_(statistics) {}

template <typename Response>
OutputStream<Response>::~OutputStream() {
  if (state_ != State::kFinished) impl::Cancel(stream_, call_name_);
}

template <typename Response>
grpc::ServerContext& OutputStream<Response>::GetContext() {
  return context_;
}

template <typename Response>
void OutputStream<Response>::Write(const Response& response) {
  UINVARIANT(state_ != State::kFinished, "'Write' called on a finished stream");

  // For some reason, gRPC requires explicit 'SendInitialMetadata' in output
  // streams
  impl::SendInitialMetadataIfNew(stream_, call_name_, state_);

  // It is safe to always buffer writes in a non-interactive stream
  const auto write_options = grpc::WriteOptions().set_buffer_hint();

  impl::Write(stream_, response, write_options, call_name_);
}

template <typename Response>
void OutputStream<Response>::Finish() {
  UINVARIANT(state_ != State::kFinished,
             "'Finish' called on a finished stream");
  state_ = State::kFinished;
  impl::Finish(stream_, grpc::Status::OK, call_name_);
  statistics_.OnExplicitFinish(grpc::StatusCode::OK);
}

template <typename Response>
void OutputStream<Response>::FinishWithError(const grpc::Status& status) {
  UASSERT(!status.ok());
  UINVARIANT(state_ != State::kFinished,
             "'Finish' called on a finished stream");
  state_ = State::kFinished;
  impl::Finish(stream_, status, call_name_);
  statistics_.OnExplicitFinish(status.error_code());
}

template <typename Response>
void OutputStream<Response>::WriteAndFinish(const Response& response) {
  UINVARIANT(state_ != State::kFinished,
             "'WriteAndFinish' called on a finished stream");
  state_ = State::kFinished;

  // It is safe to always buffer writes in a non-interactive stream
  const auto write_options = grpc::WriteOptions().set_buffer_hint();

  impl::WriteAndFinish(stream_, response, write_options, grpc::Status::OK);
}

template <typename Request, typename Response>
BidirectionalStream<Request, Response>::BidirectionalStream(
    grpc::ServerContext& context, std::string_view call_name,
    impl::RawReaderWriter<Request, Response>& stream,
    impl::RpcStatisticsScope& statistics)
    : context_(context),
      call_name_(call_name),
      stream_(stream),
      statistics_(statistics) {}

template <typename Request, typename Response>
BidirectionalStream<Request, Response>::~BidirectionalStream<Request,
                                                             Response>() {
  if (state_ != State::kFinished) impl::Cancel(stream_, call_name_);
}

template <typename Request, typename Response>
grpc::ServerContext& BidirectionalStream<Request, Response>::GetContext() {
  return context_;
}

template <typename Request, typename Response>
bool BidirectionalStream<Request, Response>::Read(Request& request) {
  UINVARIANT(state_ == State::kOpen,
             "'Read' called while the stream is half-closed for reads");
  if (impl::Read(stream_, request)) {
    return true;
  } else {
    state_ = State::kReadsDone;
    return false;
  }
}

template <typename Request, typename Response>
void BidirectionalStream<Request, Response>::Write(const Response& response) {
  UINVARIANT(state_ == State::kOpen, "'Write' called on a finished stream");

  // Don't buffer writes, optimize for ping-pong-style interaction
  grpc::WriteOptions write_options{};

  impl::Write(stream_, response, write_options, call_name_);
}

template <typename Request, typename Response>
void BidirectionalStream<Request, Response>::Finish() {
  UINVARIANT(state_ != State::kFinished,
             "'Finish' called on a finished stream");
  state_ = State::kFinished;
  impl::Finish(stream_, grpc::Status::OK, call_name_);
  statistics_.OnExplicitFinish(grpc::StatusCode::OK);
}

template <typename Request, typename Response>
void BidirectionalStream<Request, Response>::FinishWithError(
    const grpc::Status& status) {
  UASSERT(!status.ok());
  UINVARIANT(state_ != State::kFinished,
             "'FinishWithError' called on a finished stream");
  state_ = State::kFinished;
  impl::Finish(stream_, status, call_name_);
  statistics_.OnExplicitFinish(status.error_code());
}

template <typename Request, typename Response>
void BidirectionalStream<Request, Response>::WriteAndFinish(
    const Response& response) {
  UINVARIANT(state_ != State::kFinished,
             "'WriteAndFinish' called on a finished stream");
  state_ = State::kFinished;

  // Don't buffer writes, optimize for ping-pong-style interaction
  grpc::WriteOptions write_options{};

  impl::WriteAndFinish(stream_, response, write_options, grpc::Status::OK,
                       call_name_);
}

}  // namespace ugrpc::server

USERVER_NAMESPACE_END
