#pragma once

#include <memory>

#include <grpcpp/grpcpp.h>
#include <grpcpp/impl/codegen/async_stream.h>
#include <grpcpp/impl/codegen/async_unary_call.h>

#include <boost/any.hpp>

#include <logging/log.hpp>
#include <utils/fast_pimpl.hpp>

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

struct PromiseWrapperAny;
template <typename T>
struct PromiseWrapper;
template <typename T>
struct FutureWrapper;

struct FutureWrapperAny {
  FutureWrapperAny();
  FutureWrapperAny(const FutureWrapperAny&) = delete;
  FutureWrapperAny(FutureWrapperAny&&) noexcept;

  FutureWrapperAny& operator=(FutureWrapperAny&&) noexcept;

  boost::any Get();
  void Wait();

 protected:
  ~FutureWrapperAny();

  friend struct PromiseWrapperAny;
  template <typename T>
  friend struct PromiseWrapper;
  struct Impl;

  explicit FutureWrapperAny(Impl&&);
  utils::FastPimpl<Impl, 16, 8> pimpl_;
};

template <typename T>
struct FutureWrapper final : FutureWrapperAny {
  FutureWrapper() = default;
  FutureWrapper(FutureWrapper&&) noexcept = default;
  ~FutureWrapper() = default;

  FutureWrapper& operator=(FutureWrapper&&) noexcept = default;

  // Intentionally shadow base method
  T Get() { return boost::any_cast<T>(FutureWrapperAny::Get()); }

 private:
  friend struct PromiseWrapper<T>;
  FutureWrapper(FutureWrapperAny&& impl) : FutureWrapperAny(std::move(impl)) {}
};

template <>
struct FutureWrapper<void> final {
  FutureWrapper();
  FutureWrapper(const FutureWrapperAny&) = delete;
  FutureWrapper(FutureWrapper&&) noexcept;
  ~FutureWrapper();

  FutureWrapper& operator=(FutureWrapper&&) noexcept;

  void Get();
  void Wait();

 private:
  friend struct PromiseWrapper<void>;
  struct Impl;

  explicit FutureWrapper(Impl&&);
  utils::FastPimpl<Impl, 16, 8> pimpl_;
};

struct PromiseWrapperAny {
  PromiseWrapperAny();
  PromiseWrapperAny(const PromiseWrapperAny&) = delete;
  PromiseWrapperAny(PromiseWrapperAny&&) = delete;

  void SetValue(boost::any&&);
  void SetException(std::exception_ptr ex);
  FutureWrapperAny GetFuture();

 protected:
  ~PromiseWrapperAny();

 private:
  struct Impl;
  utils::FastPimpl<Impl, 16, 8> pimpl_;
};

template <typename T>
struct PromiseWrapper final : PromiseWrapperAny {
  ~PromiseWrapper() = default;
  // Intentionally shadow base method
  FutureWrapper<T> GetFuture() {
    return FutureWrapper<T>(PromiseWrapperAny::GetFuture());
  }
};

template <>
struct PromiseWrapper<void> final {
  PromiseWrapper();
  PromiseWrapper(const PromiseWrapper&) = delete;
  PromiseWrapper(PromiseWrapper&&) = delete;
  ~PromiseWrapper();

  void SetValue();
  void SetException(std::exception_ptr ex);
  FutureWrapper<void> GetFuture();

 private:
  struct Impl;
  utils::FastPimpl<Impl, 16, 8> pimpl_;
};

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
                  const Request& req)
      : context(std::move(ctx)) {
    response_reader = (stub->*prepare_func)(context.get(), req, &queue);
    response_reader->StartCall();
    response_reader->Finish(&response, &status, (void*)this);
  }

  void ProcessResult(bool finished_ok) override {
    // TODO check finished_ok
    if (!finished_ok) {
      LOG_ERROR() << "Invocation failure";
      // TODO Specific exceptions
      promise.SetException(
          std::make_exception_ptr(std::runtime_error{"Invocation failed"}));
    } else if (status.ok()) {
      promise.SetValue(std::move(response));
    } else {
      LOG_ERROR() << "Invocation failure: " << status.error_message();
      // TODO Specific exceptions
      promise.SetException(
          std::make_exception_ptr(std::runtime_error{status.error_message()}));
    }
    delete this;
  }

  std::shared_ptr<::grpc::ClientContext> context;
  Response response;
  AsyncResponseReader<Response> response_reader;
  PromiseWrapper<Response> promise;
};

template <>
struct AsyncInvocation<void> final : AsyncInvocationBase {
  // AsyncInvocation() = default;
  explicit AsyncInvocation(std::shared_ptr<::grpc::ClientContext> ctx) noexcept
      : context{std::move(ctx)} {}

  void ProcessResult(bool finished_ok) override {
    if (finished_ok) {
      // TODO check finished_ok
      promise.SetValue();
    } else {
      // TODO Specific exceptions
      promise.SetException(
          std::make_exception_ptr(std::runtime_error{"Invocation failed"}));
    }
    delete this;
  }
  std::shared_ptr<::grpc::ClientContext> context;
  PromiseWrapper<void> promise;
};

struct AsyncInvocationComplete : AsyncInvocationBase {
  AsyncInvocationComplete(std::shared_ptr<::grpc::ClientContext> ctx,
                          SharedInvocationState state)
      : context{std::move(ctx)}, state{std::move(state)} {}
  void ProcessResult(bool finished_ok) override {
    state->is_finished = true;
    delete this;
  }
  std::shared_ptr<::grpc::ClientContext> context;
  SharedInvocationState state;
};

template <typename Response>
struct AsyncValueRead final : AsyncInvocationBase {
  explicit AsyncValueRead(std::shared_ptr<::grpc::ClientContext> ctx) noexcept
      : context{std::move(ctx)} {}

  void ProcessResult(bool finished_ok) override {
    if (finished_ok) {
      promise.SetValue(std::move(response));
    } else {
      // TODO Specific exception
      promise.SetException(
          std::make_exception_ptr(std::runtime_error{"Value read failure"}));
    }
    delete this;
  }
  std::shared_ptr<::grpc::ClientContext> context;
  Response response;
  PromiseWrapper<Response> promise;
};

template <typename Response>
struct AsynIncomingStreamInvocation final : AsyncInvocationBase {
  void ProcessResult(bool finished_ok) override {}
};

/// Prepare async invocation for a simple request -> response rpc
template <typename Stub, typename Request, typename Response>
auto PrepareInvocation(Stub* stub,
                       PrepareFuncPtr<Stub, Request, Response> prepare_func,
                       ::grpc::CompletionQueue& queue,
                       std::shared_ptr<::grpc::ClientContext> ctx,
                       const Request& req) {
  auto invocation = new AsyncInvocation<Response>(stub, prepare_func, queue,
                                                  std::move(ctx), req);
  return invocation->promise.GetFuture();
}

}  // namespace clients::grpc::detail
