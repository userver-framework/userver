#pragma once

#include <memory>

#include <grpcpp/grpcpp.h>
#include <grpcpp/impl/codegen/async_stream.h>
#include <grpcpp/impl/codegen/async_unary_call.h>

#include <userver/engine/impl/blocking_future.hpp>
#include <userver/logging/log.hpp>

#include <userver/clients/grpc/errors.hpp>

namespace clients::grpc::detail {

//@{
/** @name Helper template type aliases */
template <typename Response>
using AsyncResponseReader =
    std::unique_ptr<::grpc::ClientAsyncResponseReader<Response>>;

template <typename Response>
using AsyncInStream = std::unique_ptr<::grpc::ClientAsyncReader<Response>>;

template <typename Request>
using AsyncOutStream = std::unique_ptr<::grpc::ClientAsyncWriter<Request>>;

template <typename Request, typename Response>
using AsyncBidirStream =
    std::unique_ptr<::grpc::ClientAsyncReaderWriter<Request, Response>>;

template <typename Stub, typename Request, typename Response>
using PrepareFuncPtr = AsyncResponseReader<Response> (Stub::*)(
    ::grpc::ClientContext*, const Request&, ::grpc::CompletionQueue*);

template <typename Stub, typename Request, typename Response>
using PrepareInputStreamFuncPtr = AsyncInStream<Response> (Stub::*)(
    ::grpc::ClientContext*, const Request&, ::grpc::CompletionQueue*);

template <typename Stub, typename Request, typename Response>
using PrepareOutputStreamFuncPtr = AsyncOutStream<Request> (Stub::*)(
    ::grpc::ClientContext*, Response*, ::grpc::CompletionQueue*);

template <typename Stub, typename Request, typename Response>
using PrepareBidirStreamFuncPtr = AsyncBidirStream<Request, Response> (Stub::*)(
    ::grpc::ClientContext*, ::grpc::CompletionQueue*);
//@}

struct AsyncInvocationBase {
  virtual ~AsyncInvocationBase() = default;

  virtual void ProcessResult(bool finished_ok) = 0;
};

struct AsyncInvocationWithStatus : AsyncInvocationBase {
  ::grpc::Status status;
};

struct InvocationState {
  ::grpc::Status status;
  bool is_finished = false;  // TODO Atomic?
};

using SharedInvocationState = std::shared_ptr<InvocationState>;

template <typename Response>
struct AsyncInvocation final : AsyncInvocationWithStatus {
  template <typename Stub, typename Request>
  AsyncInvocation(Stub* stub,
                  PrepareFuncPtr<Stub, Request, Response> prepare_func,
                  ::grpc::CompletionQueue& queue,
                  std::shared_ptr<::grpc::ClientContext> ctx,
                  const Request& req, std::string method_info)
      : context(std::move(ctx)), method_info(std::move(method_info)) {
    response_reader = (stub->*prepare_func)(context.get(), req, &queue);
    response_reader->StartCall();
    response_reader->Finish(&response, &status, (void*)this);
  }

  void ProcessResult(bool finished_ok) override {
    // TODO check finished_ok
    if (!finished_ok) {
      LOG_ERROR() << "Invocation failure";
      promise.set_exception(
          std::make_exception_ptr(InvocationError(method_info)));
    } else if (status.ok()) {
      promise.set_value(std::move(response));
    } else {
      LOG_ERROR() << "Invocation failure: " << status.error_message();
      promise.set_exception(StatusToExceptionPtr(status, method_info));
    }
    delete this;
  }

  std::shared_ptr<::grpc::ClientContext> context;
  Response response;
  AsyncResponseReader<Response> response_reader;
  engine::impl::BlockingPromise<Response> promise;
  const std::string method_info;
};

template <>
struct AsyncInvocation<void> final : AsyncInvocationBase {
  AsyncInvocation(std::string method_info)
      : method_info(std::move(method_info)) {}

  AsyncInvocation(SharedInvocationState state, std::string method_info) noexcept
      : state(std::move(state)), method_info(std::move(method_info)) {}

  void ProcessResult(bool finished_ok) override {
    // TODO check finished_ok
    if (state && !state->status.ok()) {
      LOG_ERROR() << "Invocation failure: " << state->status.error_message();
      promise.set_exception(StatusToExceptionPtr(state->status, method_info));
    } else if (!finished_ok) {
      LOG_ERROR() << "Invocation failure";
      promise.set_exception(
          std::make_exception_ptr(InvocationError(method_info)));
    } else {
      promise.set_value();
    }
    delete this;
  }

  SharedInvocationState state;
  engine::impl::BlockingPromise<void> promise;
  const std::string method_info;
};

struct AsyncInvocationComplete : AsyncInvocationBase {
  explicit AsyncInvocationComplete(SharedInvocationState state)
      : state{std::move(state)} {}

  void ProcessResult(bool /*finished_ok*/) override {
    state->is_finished = true;
    delete this;
  }

  SharedInvocationState state;
};

template <typename Response>
struct AsyncValueRead final : AsyncInvocationBase {
  AsyncValueRead(SharedInvocationState state, std::string method_info) noexcept
      : state(std::move(state)), method_info(std::move(method_info)) {}

  void ProcessResult(bool finished_ok) override {
    if (!state->status.ok()) {
      LOG_ERROR() << "Invocation failure: " << state->status.error_message();
      promise.set_exception(StatusToExceptionPtr(state->status, method_info));
    } else if (!finished_ok) {
      LOG_ERROR() << "Value read failure";
      promise.set_exception(
          std::make_exception_ptr(ValueReadError(method_info)));
    } else {
      promise.set_value(std::move(response));
    }
    delete this;
  }

  SharedInvocationState state;
  Response response;
  engine::impl::BlockingPromise<Response> promise;
  const std::string method_info;
};

/// Prepare async invocation for a simple request -> response rpc
template <typename Stub, typename Request, typename Response>
auto PrepareInvocation(Stub* stub,
                       PrepareFuncPtr<Stub, Request, Response> prepare_func,
                       ::grpc::CompletionQueue& queue,
                       std::shared_ptr<::grpc::ClientContext> ctx,
                       const Request& req, std::string method_info) {
  auto invocation = new AsyncInvocation<Response>(
      stub, prepare_func, queue, std::move(ctx), req, std::move(method_info));
  return invocation->promise.get_future();
}

}  // namespace clients::grpc::detail
