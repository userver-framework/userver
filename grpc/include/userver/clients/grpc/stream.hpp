#pragma once

#include <userver/clients/grpc/detail/async_invocation.hpp>

namespace clients::grpc {

template <typename Response>
class InputStream final {
 public:
  template <typename Stub, typename Request>
  InputStream(
      Stub* stub,
      detail::PrepareInputStreamFuncPtr<Stub, Request, Response> prepare_func,
      ::grpc::CompletionQueue& queue,
      std::shared_ptr<::grpc::ClientContext> ctx, const Request& req,
      std::string method_info);
  InputStream(const InputStream&) = delete;
  InputStream(InputStream&&) = default;

  explicit operator bool() const { return !IsReadFinished(); }
  bool IsReadFinished() const { return state_->is_finished; }

  Response Read();

 private:
  auto ReadNext();

 private:
  std::shared_ptr<::grpc::ClientContext> context_;
  detail::SharedInvocationState state_;

  detail::AsyncInStream<Response> stream_;
  detail::FutureWrapper<Response> next_value_;

  const std::string method_info_;
};

template <typename Request, typename Response>
class OutputStream final {
 public:
  template <typename Stub>
  OutputStream(
      Stub* stub,
      detail::PrepareOutputStreamFuncPtr<Stub, Request, Response> prepare_func,
      ::grpc::CompletionQueue& queue,
      std::shared_ptr<::grpc::ClientContext> ctx, std::string method_info);
  OutputStream(const OutputStream&) = delete;
  OutputStream(OutputStream&&) = default;

  ~OutputStream() { FinishWrites(); }

  explicit operator bool() const { return !writes_done_; }

  void Write(const Request& req);
  void FinishWrites();

  Response GetResponse() {
    FinishWrites();
    return response_.Get();
  }

 private:
  std::shared_ptr<::grpc::ClientContext> context_;
  detail::SharedInvocationState state_;
  bool writes_done_ = false;

  detail::AsyncOutStream<Request> stream_;
  detail::FutureWrapper<Response> response_;

  const std::string method_info_;
};

template <typename Request, typename Response>
class BidirStream final {
 public:
  template <typename Stub>
  BidirStream(
      Stub* stub,
      detail::PrepareBidirStreamFuncPtr<Stub, Request, Response> prepare_func,
      ::grpc::CompletionQueue& queue,
      std::shared_ptr<::grpc::ClientContext> ctx, std::string method_info);
  BidirStream(const BidirStream&) = delete;
  BidirStream(BidirStream&&) = default;

  ~BidirStream() { FinishWrites(); }

  Response Read();
  bool IsReadFinished() const { return state_->is_finished; }

  void Write(const Request& req);
  void FinishWrites();
  bool IsWriteFinished() const { return writes_done_; }

 private:
  auto ReadNext();

 private:
  std::shared_ptr<::grpc::ClientContext> context_;
  detail::SharedInvocationState state_;
  bool writes_done_ = false;

  detail::AsyncBidirStream<Request, Response> stream_;
  detail::FutureWrapper<Response> next_value_;

  const std::string method_info_;
};

// Implementation follows
// InputStream functions
template <typename Response>
template <typename Stub, typename Request>
InputStream<Response>::InputStream(
    Stub* stub,
    detail::PrepareInputStreamFuncPtr<Stub, Request, Response> prepare_func,
    ::grpc::CompletionQueue& queue, std::shared_ptr<::grpc::ClientContext> ctx,
    const Request& req, std::string method_info)
    : context_{std::move(ctx)},
      state_{std::make_shared<detail::InvocationState>()},
      method_info_(std::move(method_info)) {
  stream_ = (stub->*prepare_func)(context_.get(), req, &queue);

  auto invocation_done = new detail::AsyncInvocation<void>{method_info_};
  auto started = invocation_done->promise.GetFuture();
  stream_->StartCall(invocation_done);

  // don't track metadata received yet
  stream_->ReadInitialMetadata(nullptr);

  auto status = new detail::AsyncInvocationComplete{state_};
  stream_->Finish(&state_->status, status);

  started.Get();
  next_value_ = ReadNext();
  next_value_.Wait();
}

template <typename Response>
Response InputStream<Response>::Read() {
  Response val = next_value_.Get();
  next_value_ = ReadNext();
  return val;
}

template <typename Response>
auto InputStream<Response>::ReadNext() {
  auto invocation = new detail::AsyncValueRead<Response>{state_, method_info_};
  auto future = invocation->promise.GetFuture();
  stream_->Read(&invocation->response, invocation);
  return future;
}

template <typename Response>
InputStream<Response>& operator>>(InputStream<Response>& is, Response& val) {
  val = is.Read();
  return is;
}

// OutputStream functions
template <typename Request, typename Response>
template <typename Stub>
OutputStream<Request, Response>::OutputStream(
    Stub* stub,
    detail::PrepareOutputStreamFuncPtr<Stub, Request, Response> prepare_func,
    ::grpc::CompletionQueue& queue, std::shared_ptr<::grpc::ClientContext> ctx,
    std::string method_info)
    : context_{std::move(ctx)},
      state_{std::make_shared<detail::InvocationState>()},
      method_info_(std::move(method_info)) {
  auto invocation = new detail::AsyncValueRead<Response>{state_, method_info_};
  response_ = invocation->promise.GetFuture();
  stream_ =
      (stub->*prepare_func)(context_.get(), &invocation->response, &queue);

  auto invocation_done = new detail::AsyncInvocation<void>{method_info_};
  auto started = invocation_done->promise.GetFuture();
  stream_->StartCall(invocation_done);

  // don't track metadata received yet
  stream_->ReadInitialMetadata(nullptr);

  stream_->Finish(&state_->status, invocation);
  started.Get();
}

template <typename Request, typename Response>
void OutputStream<Request, Response>::Write(const Request& req) {
  if (!writes_done_) {
    auto invocation_done =
        new detail::AsyncInvocation<void>{state_, method_info_};
    auto finished = invocation_done->promise.GetFuture();
    stream_->Write(req, invocation_done);
    finished.Get();
  } else {
    throw StreamClosedError(method_info_);
  }
}

template <typename Request, typename Response>
void OutputStream<Request, Response>::FinishWrites() {
  if (stream_ && !writes_done_) {
    writes_done_ = true;
    auto invocation_done = new detail::AsyncInvocation<void>{method_info_};
    auto finished = invocation_done->promise.GetFuture();
    stream_->WritesDone(invocation_done);
    finished.Get();
  }
}

template <typename Request, typename Response>
OutputStream<Request, Response>& operator<<(OutputStream<Request, Response>& os,
                                            const Request& req) {
  os.Write(req);
  return os;
}

// Bidir functions
template <typename Request, typename Response>
template <typename Stub>
BidirStream<Request, Response>::BidirStream(
    Stub* stub,
    detail::PrepareBidirStreamFuncPtr<Stub, Request, Response> prepare_func,
    ::grpc::CompletionQueue& queue, std::shared_ptr<::grpc::ClientContext> ctx,
    std::string method_info)
    : context_{std::move(ctx)},
      state_{std::make_shared<detail::InvocationState>()},
      method_info_(std::move(method_info)) {
  stream_ = (stub->*prepare_func)(context_.get(), &queue);

  auto invocation_done = new detail::AsyncInvocation<void>{method_info_};
  auto started = invocation_done->promise.GetFuture();
  stream_->StartCall(invocation_done);

  // don't track metadata received yet
  stream_->ReadInitialMetadata(nullptr);

  auto status = new detail::AsyncInvocationComplete{state_};
  stream_->Finish(&state_->status, status);

  started.Get();
  next_value_ = ReadNext();
}

template <typename Request, typename Response>
Response BidirStream<Request, Response>::Read() {
  Response val = next_value_.Get();
  next_value_ = ReadNext();
  return val;
}

template <typename Request, typename Response>
auto BidirStream<Request, Response>::ReadNext() {
  auto invocation = new detail::AsyncValueRead<Response>{state_, method_info_};
  auto future = invocation->promise.GetFuture();
  stream_->Read(&invocation->response, invocation);
  return future;
}

template <typename Request, typename Response>
void BidirStream<Request, Response>::Write(const Request& req) {
  if (!writes_done_) {
    auto invocation_done =
        new detail::AsyncInvocation<void>{state_, method_info_};
    auto finished = invocation_done->promise.GetFuture();
    stream_->Write(req, invocation_done);
    finished.Get();
  } else {
    throw StreamClosedError(method_info_);
  }
}

template <typename Request, typename Response>
void BidirStream<Request, Response>::FinishWrites() {
  if (stream_ && !writes_done_) {
    writes_done_ = true;
    auto invocation_done = new detail::AsyncInvocation<void>{method_info_};
    auto finished = invocation_done->promise.GetFuture();
    stream_->WritesDone(invocation_done);
    finished.Get();
  }
}

template <typename Request, typename Response>
struct BidirReadHelper {
  BidirStream<Request, Response>& stream;

  explicit operator bool() const { return !stream.IsReadFinished(); }
};

template <typename Request, typename Response>
struct BidirWriteHelper {
  BidirStream<Request, Response>& stream;

  explicit operator bool() const { return !stream.IsWriteFinished(); }
};

template <typename Request, typename Response>
BidirWriteHelper<Request, Response> operator<<(
    BidirStream<Request, Response>& os, const Request& req) {
  os.Write(req);
  return {os};
}

template <typename Request, typename Response>
BidirWriteHelper<Request, Response> operator<<(
    BidirWriteHelper<Request, Response> os, const Request& req) {
  return os.stream << req;
}

template <typename Request, typename Response>
BidirReadHelper<Request, Response> operator>>(
    BidirStream<Request, Response>& is, Response& resp) {
  resp = is.Read();
  return {is};
}

template <typename Request, typename Response>
BidirReadHelper<Request, Response> operator>>(
    BidirReadHelper<Request, Response> is, Response& resp) {
  return is.stream >> resp;
}

namespace detail {

/// Prepare async invocation for a single request -> response stream rpc
template <typename Stub, typename Request, typename Response>
auto PrepareInvocation(
    Stub* stub, PrepareInputStreamFuncPtr<Stub, Request, Response> prepare_func,
    ::grpc::CompletionQueue& queue, std::shared_ptr<::grpc::ClientContext> ctx,
    const Request& req, std::string method_info) {
  return InputStream<Response>{
      stub, prepare_func, queue, std::move(ctx), req, std::move(method_info)};
}

/// Prepare async invocation for a request stream -> single response rpc
template <typename Stub, typename Request, typename Response>
auto PrepareInvocation(
    Stub* stub,
    PrepareOutputStreamFuncPtr<Stub, Request, Response> prepare_func,
    ::grpc::CompletionQueue& queue, std::shared_ptr<::grpc::ClientContext> ctx,
    std::string method_info) {
  return OutputStream<Request, Response>{
      stub, prepare_func, queue, std::move(ctx), std::move(method_info)};
};

/// Prepare bidirectional stream
template <typename Stub, typename Request, typename Response>
auto PrepareInvocation(
    Stub* stub, PrepareBidirStreamFuncPtr<Stub, Request, Response> prepare_func,
    ::grpc::CompletionQueue& queue, std::shared_ptr<::grpc::ClientContext> ctx,
    std::string method_info) {
  return BidirStream<Request, Response>{stub, prepare_func, queue,
                                        std::move(ctx), std::move(method_info)};
};

}  // namespace detail

}  // namespace clients::grpc
