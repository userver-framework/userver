#pragma once

/// @file userver/ugrpc/server/rpc.hpp
/// @brief Classes representing an incoming RPC

#include <grpcpp/impl/codegen/proto_utils.h>
#include <grpcpp/server_context.h>

#include <userver/utils/assert.hpp>

#include <userver/ugrpc/impl/deadline_timepoint.hpp>
#include <userver/ugrpc/impl/internal_tag_fwd.hpp>
#include <userver/ugrpc/impl/span.hpp>
#include <userver/ugrpc/impl/statistics_scope.hpp>
#include <userver/ugrpc/server/exceptions.hpp>
#include <userver/ugrpc/server/impl/async_methods.hpp>
#include <userver/ugrpc/server/impl/call_params.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server {

/// @brief A non-typed base class for any gRPC call
class CallAnyBase {
 public:
  CallAnyBase(impl::CallParams&& params) : params_(params) {}

  /// @brief Complete the RPC with an error
  ///
  /// `Finish` must not be called multiple times for the same RPC.
  ///
  /// @param status error details
  /// @throws ugrpc::server::RpcError on an RPC error
  virtual void FinishWithError(const grpc::Status& status) = 0;

  /// @returns the `ServerContext` used for this RPC
  /// @note Initial server metadata is not currently supported
  /// @note Trailing metadata, if any, must be set before the `Finish` call
  grpc::ServerContext& GetContext() { return params_.context; }

  /// @brief Name of the call. Consists of service and method names
  std::string_view GetCallName() const { return params_.call_name; }

  tracing::Span& GetCallSpan() { return params_.call_span; }

  /// @cond
  // For internal use only
  ugrpc::impl::RpcStatisticsScope& Statistics(ugrpc::impl::InternalTag);
  /// @endcond

 protected:
  ugrpc::impl::RpcStatisticsScope& Statistics() { return params_.statistics; }

  logging::LoggerCRef AccessTskvLogger() { return params_.access_tskv_logger; }

  void LogFinish(grpc::Status status) const;

 private:
  impl::CallParams params_;
};

/// @brief Controls a single request -> single response RPC
///
/// The RPC is cancelled on destruction unless `Finish` has been called.
template <typename Response>
class UnaryCall final : public CallAnyBase {
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
  void FinishWithError(const grpc::Status& status) override;

  /// For internal use only
  UnaryCall(impl::CallParams&& call_params,
            impl::RawResponseWriter<Response>& stream);

  UnaryCall(UnaryCall&&) = delete;
  UnaryCall& operator=(UnaryCall&&) = delete;
  ~UnaryCall();

 private:
  impl::RawResponseWriter<Response>& stream_;
  bool is_finished_{false};
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
class InputStream final : public CallAnyBase {
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
  void FinishWithError(const grpc::Status& status) override;

  /// For internal use only
  InputStream(impl::CallParams&& call_params,
              impl::RawReader<Request, Response>& stream);

  InputStream(InputStream&&) = delete;
  InputStream& operator=(InputStream&&) = delete;
  ~InputStream();

 private:
  enum class State { kOpen, kReadsDone, kFinished };

  impl::RawReader<Request, Response>& stream_;
  State state_{State::kOpen};
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
class OutputStream final : public CallAnyBase {
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
  void FinishWithError(const grpc::Status& status) override;

  /// @brief Equivalent to `Write + Finish`
  ///
  /// This call saves one round-trip, compared to separate `Write` and `Finish`.
  ///
  /// `Finish` must not be called multiple times.
  ///
  /// @param response the final response message
  /// @throws ugrpc::server::RpcError on an RPC error
  void WriteAndFinish(const Response& response);

  /// For internal use only
  OutputStream(impl::CallParams&& call_params,
               impl::RawWriter<Response>& stream);

  OutputStream(OutputStream&&) = delete;
  OutputStream& operator=(OutputStream&&) = delete;
  ~OutputStream();

 private:
  enum class State { kNew, kOpen, kFinished };

  impl::RawWriter<Response>& stream_;
  State state_{State::kNew};
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
class BidirectionalStream : public CallAnyBase {
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
  void FinishWithError(const grpc::Status& status) override;

  /// @brief Equivalent to `Write + Finish`
  ///
  /// This call saves one round-trip, compared to separate `Write` and `Finish`.
  ///
  /// `Finish` must not be called multiple times.
  ///
  /// @param response the final response message
  /// @throws ugrpc::server::RpcError on an RPC error
  void WriteAndFinish(const Response& response);

  /// For internal use only
  BidirectionalStream(impl::CallParams&& call_params,
                      impl::RawReaderWriter<Request, Response>& stream);

  BidirectionalStream(const BidirectionalStream&) = delete;
  BidirectionalStream(BidirectionalStream&&) = delete;
  ~BidirectionalStream();

 private:
  enum class State { kOpen, kReadsDone, kFinished };

  impl::RawReaderWriter<Request, Response>& stream_;
  State state_{State::kOpen};
};

// ========================== Implementation follows ==========================

template <typename Response>
UnaryCall<Response>::UnaryCall(impl::CallParams&& call_params,
                               impl::RawResponseWriter<Response>& stream)
    : CallAnyBase(std::move(call_params)), stream_(stream) {}

template <typename Response>
UnaryCall<Response>::~UnaryCall() {
  if (!is_finished_) {
    impl::CancelWithError(stream_, GetCallName());
    LogFinish(impl::kUnknownErrorStatus);
  }
}

template <typename Response>
void UnaryCall<Response>::Finish(const Response& response) {
  UINVARIANT(!is_finished_, "'Finish' called on a finished call");
  is_finished_ = true;

  LogFinish(grpc::Status::OK);
  impl::Finish(stream_, response, grpc::Status::OK, GetCallName());
  Statistics().OnExplicitFinish(grpc::StatusCode::OK);
  ugrpc::impl::UpdateSpanWithStatus(GetCallSpan(), grpc::Status::OK);
}

template <typename Response>
void UnaryCall<Response>::FinishWithError(const grpc::Status& status) {
  UINVARIANT(!is_finished_, "'FinishWithError' called on a finished call");
  is_finished_ = true;
  LogFinish(status);
  impl::FinishWithError(stream_, status, GetCallName());
  Statistics().OnExplicitFinish(status.error_code());
  ugrpc::impl::UpdateSpanWithStatus(GetCallSpan(), status);
}

template <typename Request, typename Response>
InputStream<Request, Response>::InputStream(
    impl::CallParams&& call_params, impl::RawReader<Request, Response>& stream)
    : CallAnyBase(std::move(call_params)), stream_(stream) {}

template <typename Request, typename Response>
InputStream<Request, Response>::~InputStream() {
  if (state_ != State::kFinished) {
    impl::CancelWithError(stream_, GetCallName());
    LogFinish(impl::kUnknownErrorStatus);
  }
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
  LogFinish(grpc::Status::OK);
  impl::Finish(stream_, response, grpc::Status::OK, GetCallName());
  Statistics().OnExplicitFinish(grpc::StatusCode::OK);
  ugrpc::impl::UpdateSpanWithStatus(GetCallSpan(), grpc::Status::OK);
}

template <typename Request, typename Response>
void InputStream<Request, Response>::FinishWithError(
    const grpc::Status& status) {
  UASSERT(!status.ok());
  UINVARIANT(state_ != State::kFinished,
             "'FinishWithError' called on a finished stream");
  state_ = State::kFinished;
  LogFinish(status);
  impl::FinishWithError(stream_, status, GetCallName());
  Statistics().OnExplicitFinish(status.error_code());
  ugrpc::impl::UpdateSpanWithStatus(GetCallSpan(), status);
}

template <typename Response>
OutputStream<Response>::OutputStream(impl::CallParams&& call_params,
                                     impl::RawWriter<Response>& stream)
    : CallAnyBase(std::move(call_params)), stream_(stream) {}

template <typename Response>
OutputStream<Response>::~OutputStream() {
  if (state_ != State::kFinished) {
    impl::Cancel(stream_, GetCallName());
    LogFinish(impl::kUnknownErrorStatus);
  }
}

template <typename Response>
void OutputStream<Response>::Write(const Response& response) {
  UINVARIANT(state_ != State::kFinished, "'Write' called on a finished stream");

  // For some reason, gRPC requires explicit 'SendInitialMetadata' in output
  // streams
  impl::SendInitialMetadataIfNew(stream_, GetCallName(), state_);

  // Don't buffer writes, otherwise in an event subscription scenario, events
  // may never actually be delivered
  grpc::WriteOptions write_options{};

  impl::Write(stream_, response, write_options, GetCallName());
}

template <typename Response>
void OutputStream<Response>::Finish() {
  UINVARIANT(state_ != State::kFinished,
             "'Finish' called on a finished stream");
  state_ = State::kFinished;
  const auto status = grpc::Status::OK;
  LogFinish(status);
  impl::Finish(stream_, status, GetCallName());
  Statistics().OnExplicitFinish(grpc::StatusCode::OK);
  ugrpc::impl::UpdateSpanWithStatus(GetCallSpan(), status);
}

template <typename Response>
void OutputStream<Response>::FinishWithError(const grpc::Status& status) {
  UASSERT(!status.ok());
  UINVARIANT(state_ != State::kFinished,
             "'Finish' called on a finished stream");
  state_ = State::kFinished;
  LogFinish(status);
  impl::Finish(stream_, status, GetCallName());
  Statistics().OnExplicitFinish(status.error_code());
  ugrpc::impl::UpdateSpanWithStatus(GetCallSpan(), status);
}

template <typename Response>
void OutputStream<Response>::WriteAndFinish(const Response& response) {
  UINVARIANT(state_ != State::kFinished,
             "'WriteAndFinish' called on a finished stream");
  state_ = State::kFinished;

  // Don't buffer writes, otherwise in an event subscription scenario, events
  // may never actually be delivered
  grpc::WriteOptions write_options{};

  const auto status = grpc::Status::OK;
  LogFinish(status);
  impl::WriteAndFinish(stream_, response, write_options, status, GetCallName());
}

template <typename Request, typename Response>
BidirectionalStream<Request, Response>::BidirectionalStream(
    impl::CallParams&& call_params,
    impl::RawReaderWriter<Request, Response>& stream)
    : CallAnyBase(std::move(call_params)), stream_(stream) {}

template <typename Request, typename Response>
BidirectionalStream<Request, Response>::~BidirectionalStream() {
  if (state_ != State::kFinished) {
    impl::Cancel(stream_, GetCallName());
    LogFinish(impl::kUnknownErrorStatus);
  }
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

  impl::Write(stream_, response, write_options, GetCallName());
}

template <typename Request, typename Response>
void BidirectionalStream<Request, Response>::Finish() {
  UINVARIANT(state_ != State::kFinished,
             "'Finish' called on a finished stream");
  state_ = State::kFinished;
  const auto status = grpc::Status::OK;
  LogFinish(status);
  impl::Finish(stream_, status, GetCallName());
  Statistics().OnExplicitFinish(grpc::StatusCode::OK);
  ugrpc::impl::UpdateSpanWithStatus(GetCallSpan(), status);
}

template <typename Request, typename Response>
void BidirectionalStream<Request, Response>::FinishWithError(
    const grpc::Status& status) {
  UASSERT(!status.ok());
  UINVARIANT(state_ != State::kFinished,
             "'FinishWithError' called on a finished stream");
  state_ = State::kFinished;
  LogFinish(status);
  impl::Finish(stream_, status, GetCallName());
  Statistics().OnExplicitFinish(status.error_code());
  ugrpc::impl::UpdateSpanWithStatus(GetCallSpan(), status);
}

template <typename Request, typename Response>
void BidirectionalStream<Request, Response>::WriteAndFinish(
    const Response& response) {
  UINVARIANT(state_ != State::kFinished,
             "'WriteAndFinish' called on a finished stream");
  state_ = State::kFinished;

  // Don't buffer writes, optimize for ping-pong-style interaction
  grpc::WriteOptions write_options{};

  const auto status = grpc::Status::OK;
  LogFinish(status);
  impl::WriteAndFinish(stream_, response, write_options, status, GetCallName());
}

}  // namespace ugrpc::server

USERVER_NAMESPACE_END
