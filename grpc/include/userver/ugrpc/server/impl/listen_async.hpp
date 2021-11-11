#pragma once

#include <exception>
#include <string_view>
#include <type_traits>
#include <utility>

#include <grpcpp/completion_queue.h>
#include <grpcpp/server_context.h>

#include <userver/engine/async.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utils/lazy_prvalue.hpp>

#include <userver/ugrpc/impl/async_method_invocation.hpp>
#include <userver/ugrpc/server/impl/reactor_data.hpp>
#include <userver/ugrpc/server/rpc.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::impl {

/// @{
/// @brief Helper type aliases for stub member function pointers
template <typename Stub, typename Request, typename Response>
using RawResponseReaderPreparer = void (Stub::*)(
    ::grpc::ServerContext*, Request*, RawResponseWriter<Response>*,
    ::grpc::CompletionQueue* new_call_cq,
    ::grpc::ServerCompletionQueue* notification_cq, void* tag);

template <typename Stub, typename Request, typename Response>
using RawReaderPreparer =
    void (Stub::*)(::grpc::ServerContext*, RawReader<Request, Response>*,
                   ::grpc::CompletionQueue* new_call_cq,
                   ::grpc::ServerCompletionQueue* notification_cq, void* tag);

template <typename Stub, typename Request, typename Response>
using RawWriterPreparer =
    void (Stub::*)(::grpc::ServerContext*, Request*, RawWriter<Response>*,
                   ::grpc::CompletionQueue* new_call_cq,
                   ::grpc::ServerCompletionQueue* notification_cq, void* tag);

template <typename Stub, typename Request, typename Response>
using RawReaderWriterPreparer =
    void (Stub::*)(::grpc::ServerContext*, RawReaderWriter<Request, Response>*,
                   ::grpc::CompletionQueue* new_call_cq,
                   ::grpc::ServerCompletionQueue* notification_cq, void* tag);
/// @}

/// @{
/// @brief Helper type aliases for handler member function pointers
template <typename Handler, typename Request, typename Response>
using ResponseReaderHandlerFunc = void (Handler::*)(UnaryCall<Response>&,
                                                    Request&&);

template <typename Handler, typename Request, typename Response>
using ReaderHandlerFunc = void (Handler::*)(InputStream<Request, Response>&);

template <typename Handler, typename Request, typename Response>
using WriterHandlerFunc = void (Handler::*)(OutputStream<Response>&, Request&&);

template <typename Handler, typename Request, typename Response>
using ReaderWriterHandlerFunc =
    void (Handler::*)(BidirectionalStream<Request, Response>&);
/// @}

void ReportHandlerError(const std::exception& ex,
                        std::string_view call_name) noexcept;

void ReportNonStdHandlerError(std::string_view call_name) noexcept;

struct NoInitialRequest final {};

template <typename RawResponder, typename InitialRequest>
class CallData final {
 public:
  template <typename Handler, typename PrepareFuncType>
  CallData(ReactorData<Handler>& reactor, PrepareFuncType prepare_func) {
    auto& stub = reactor.GetStub();
    auto& queue = reactor.GetQueue();

    if constexpr (std::is_same_v<InitialRequest, NoInitialRequest>) {
      (stub.*prepare_func)(&context_, &raw_responder_, &queue, &queue,
                           prepare_.GetTag());
    } else {
      (stub.*prepare_func)(&context_, &initial_request_, &raw_responder_,
                           &queue, &queue, prepare_.GetTag());
    }
  }

  bool WaitForIncomingRequest() noexcept { return prepare_.Wait(); }

  template <typename Responder, typename Handler, typename HandlerFuncType>
  void HandleRpc(ReactorData<Handler>& reactor, std::string_view call_name,
                 HandlerFuncType handler_func) noexcept {
    try {
      tracing::Span span(std::string{call_name});
      Responder responder(context_, call_name, raw_responder_);
      auto& handler = reactor.GetHandler();

      if constexpr (std::is_same_v<InitialRequest, NoInitialRequest>) {
        (handler.*handler_func)(responder);
      } else {
        (handler.*handler_func)(responder, std::move(initial_request_));
      }
    } catch (const std::exception& ex) {
      ReportHandlerError(ex, call_name);
    } catch (...) {
      ReportNonStdHandlerError(call_name);
    }
  }

 private:
  ::grpc::ServerContext context_{};
  InitialRequest initial_request_{};
  RawResponder raw_responder_{&context_};
  AsyncMethodInvocation prepare_{};
};

template <typename Responder, typename RawResponder, typename InitialRequest,
          typename Handler, typename PrepareFuncType, typename HandlerFuncType>
void DoListenAsync(ReactorData<Handler>& reactor, std::string_view call_name,
                   PrepareFuncType prepare_func, HandlerFuncType handler_func) {
  using CallDataType = CallData<RawResponder, InitialRequest>;

  engine::CriticalAsyncNoSpan(
      reactor.GetHandlerTaskProcessor(),
      [&reactor, call_name, prepare_func, handler_func,
       token = reactor.GetWaitToken()](CallDataType&& call_data) {
        if (!call_data.WaitForIncomingRequest()) {
          // the CompletionQueue is shutting down
          return;
        }

        // start a concurrent listener immediately, as advised by gRPC docs
        DoListenAsync<Responder, RawResponder, InitialRequest>(
            reactor, call_name, prepare_func, handler_func);

        call_data.template HandleRpc<Responder>(reactor, call_name,
                                                handler_func);
      },
      // the request for an incoming RPC must be performed synchronously
      utils::LazyPrvalue([&] { return CallDataType(reactor, prepare_func); }))
      .Detach();
}

template <typename Handler, typename Stub, typename Request, typename Response>
void ListenAsync(
    ReactorData<Handler>& reactor, std::string_view call_name,
    RawResponseReaderPreparer<Stub, Request, Response> prepare_func,
    ResponseReaderHandlerFunc<Handler, Request, Response> handler_func) {
  return DoListenAsync<UnaryCall<Response>, RawResponseWriter<Response>,
                       Request>(reactor, call_name, prepare_func, handler_func);
}

template <typename Handler, typename Stub, typename Request, typename Response>
void ListenAsync(ReactorData<Handler>& reactor, std::string_view call_name,
                 RawReaderPreparer<Stub, Request, Response> prepare_func,
                 ReaderHandlerFunc<Handler, Request, Response> handler_func) {
  return DoListenAsync<InputStream<Request, Response>,
                       RawReader<Request, Response>, NoInitialRequest>(
      reactor, call_name, prepare_func, handler_func);
}

template <typename Handler, typename Stub, typename Request, typename Response>
void ListenAsync(ReactorData<Handler>& reactor, std::string_view call_name,
                 RawWriterPreparer<Stub, Request, Response> prepare_func,
                 WriterHandlerFunc<Handler, Request, Response> handler_func) {
  return DoListenAsync<OutputStream<Response>, RawWriter<Response>, Request>(
      reactor, call_name, prepare_func, handler_func);
}

template <typename Handler, typename Stub, typename Request, typename Response>
void ListenAsync(
    ReactorData<Handler>& reactor, std::string_view call_name,
    RawReaderWriterPreparer<Stub, Request, Response> prepare_func,
    ReaderWriterHandlerFunc<Handler, Request, Response> handler_func) {
  return DoListenAsync<BidirectionalStream<Request, Response>,
                       RawReaderWriter<Request, Response>, NoInitialRequest>(
      reactor, call_name, prepare_func, handler_func);
}

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
