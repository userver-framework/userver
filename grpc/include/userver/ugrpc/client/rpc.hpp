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

/// @brief UnaryFuture for waiting a single response RPC
class UnaryFuture {
 public:
  /// @cond
  explicit UnaryFuture(impl::RpcData& data) noexcept;
  /// @endcond

  ~UnaryFuture() noexcept;

  UnaryFuture(UnaryFuture&&) noexcept;
  UnaryFuture& operator=(UnaryFuture&&) noexcept;

  /// @brief Await response
  ///
  /// Upon completion result is available in `response` when initiating the
  /// asynchronous operation, e.g. FinishAsync.
  ///
  /// `Get` should not be called multiple times for the same RPC.
  ///
  /// @throws ugrpc::client::RpcError on an RPC error
  void Get();

 private:
  impl::RpcData* data_;
};

/// @brief Controls a single request -> single response RPC
///
/// This class is not thread-safe except for `GetContext`.
///
/// The RPC is cancelled on destruction unless `Finish` or `FinishAsync`. In
/// that case the connection is not closed (it will be reused for new RPCs), and
/// the server receives `RpcInterruptedError` immediately. Only one method of
/// `Finish` or `FinishAsync`should be called for the same RPC.
template <typename Response>
class USERVER_NODISCARD UnaryCall final {
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

  /// For internal use only
  template <typename Stub, typename Request>
  UnaryCall(
      Stub& stub, grpc::CompletionQueue& queue,
      impl::RawResponseReaderPreparer<Stub, Request, Response> prepare_func,
      std::string_view call_name, std::unique_ptr<grpc::ClientContext> context,
      ugrpc::impl::MethodStatistics& statistics, const Request& req);

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
/// returned `false` or `Finish` has been called). In that case the connection
/// is not closed (it will be reused for new RPCs), and the server receives
/// `RpcInterruptedError` immediately.
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

  /// @brief Complete the RPC successfully
  ///
  /// Should be called if `Read` has not returned `false`, but all the needed
  /// data is read.
  ///
  /// `Finish` should not be called multiple times.
  ///
  /// The connection is not closed, it will be reused for new RPCs.
  ///
  /// @throws ugrpc::client::RpcError on an RPC error
  void Finish();

  /// @returns the `ClientContext` used for this RPC
  grpc::ClientContext& GetContext();

  /// For internal use only
  template <typename Stub, typename Request>
  InputStream(Stub& stub, grpc::CompletionQueue& queue,
              impl::RawReaderPreparer<Stub, Request, Response> prepare_func,
              std::string_view call_name,
              std::unique_ptr<grpc::ClientContext> context,
              ugrpc::impl::MethodStatistics& statistics, const Request& req);

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
  /// `Finish` should not be called multiple times.
  ///
  /// The connection is not closed, it will be reused for new RPCs.
  ///
  /// @returns the single `Response` received after finishing the writes
  /// @throws ugrpc::client::RpcError on an RPC error
  Response Finish();

  /// @returns the `ClientContext` used for this RPC
  grpc::ClientContext& GetContext();

  /// For internal use only
  template <typename Stub>
  OutputStream(Stub& stub, grpc::CompletionQueue& queue,
               impl::RawWriterPreparer<Stub, Request, Response> prepare_func,
               std::string_view call_name,
               std::unique_ptr<grpc::ClientContext> context,
               ugrpc::impl::MethodStatistics& statistics);

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
/// This class is not thread-safe except for `GetContext`.
///
/// The RPC is cancelled on destruction unless the stream is closed (`Read` has
/// returned `false` or `Finish` has been called). In that case the connection
/// is not closed (it will be reused for new RPCs), and the server receives
/// `RpcInterruptedError` immediately.
///
/// If any method throws, further methods must not be called on the same stream,
/// except for `GetContext`.
template <typename Request, typename Response>
class USERVER_NODISCARD BidirectionalStream final {
 public:
  /// @brief Await and read the next incoming message
  ///
  /// On end-of-input, `Finish` is called automatically.
  ///
  /// @param response where to put response on success
  /// @returns `true` on success, `false` on end-of-input
  /// @throws ugrpc::client::RpcError on an RPC error
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

  /// @brief Complete the RPC successfully
  ///
  /// Should be called if `Read` has not returned `false`, but all the needed
  /// data is read.
  ///
  /// `Finish` should not be called multiple times.
  ///
  /// The connection is not closed, it will be reused for new RPCs.
  ///
  /// @throws ugrpc::client::RpcError on an RPC error
  void Finish();

  /// @returns the `ClientContext` used for this RPC
  grpc::ClientContext& GetContext();

  /// For internal use only
  template <typename Stub>
  BidirectionalStream(
      Stub& stub, grpc::CompletionQueue& queue,
      impl::RawReaderWriterPreparer<Stub, Request, Response> prepare_func,
      std::string_view call_name, std::unique_ptr<grpc::ClientContext> context,
      ugrpc::impl::MethodStatistics& statistics);

  BidirectionalStream(BidirectionalStream&&) noexcept = default;
  BidirectionalStream& operator=(BidirectionalStream&&) noexcept = default;
  ~BidirectionalStream() = default;

 private:
  std::unique_ptr<impl::RpcData> data_;
  impl::RawReaderWriter<Request, Response> stream_;
};

// ========================== Implementation follows ==========================

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
  data_->SetState(impl::State::kWritesDone);
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
  data_->SetState(impl::State::kWritesDone);
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
    impl::Finish(*stream_, *data_);
    return false;
  }
}

template <typename Response>
void InputStream<Response>::Finish() {
  impl::Finish(*stream_, *data_);
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
void OutputStream<Request, Response>::Write(const Request& request) {
  // Don't buffer writes, otherwise in an event subscription scenario, events
  // may never actually be delivered
  grpc::WriteOptions write_options{};

  impl::Write(*stream_, request, write_options, *data_);
}

template <typename Request, typename Response>
Response OutputStream<Request, Response>::Finish() {
  // gRPC does not implicitly call `WritesDone` in `Finish`,
  // contrary to the documentation
  impl::WritesDone(*stream_, *data_);

  impl::Finish(*stream_, *data_);

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
bool BidirectionalStream<Request, Response>::Read(Response& response) {
  if (impl::Read(*stream_, response, *data_)) {
    return true;
  } else {
    impl::Finish(*stream_, *data_);
    return false;
  }
}

template <typename Request, typename Response>
void BidirectionalStream<Request, Response>::Write(const Request& request) {
  // Don't buffer writes, optimize for ping-pong-style interaction
  grpc::WriteOptions write_options{};

  impl::Write(*stream_, request, write_options, *data_);
}

template <typename Request, typename Response>
void BidirectionalStream<Request, Response>::WritesDone() {
  impl::WritesDone(*stream_, *data_);
}

template <typename Request, typename Response>
void BidirectionalStream<Request, Response>::Finish() {
  if (data_->GetState() == impl::State::kOpen) {
    impl::WritesDone(*stream_, *data_);
  }
  impl::Finish(*stream_, *data_);
}

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
