#pragma once

#include <exception>
#include <string_view>
#include <utility>

#include <grpcpp/completion_queue.h>
#include <grpcpp/server_context.h>

#include <userver/engine/async.hpp>
#include <userver/tracing/span.hpp>

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

void ReportHandlerError(const std::exception& ex, std::string_view call_name);

void ReportNonStdHandlerError(std::string_view call_name);

template <typename Handler, typename HandlerFunc, typename... Args>
void WrapHandlerCall(std::string_view call_name, Handler& handler,
                     HandlerFunc handler_func, Args&&... args) {
  tracing::Span span(std::string{call_name});
  try {
    (handler.*handler_func)(std::forward<Args>(args)...);
  } catch (const std::exception& ex) {
    ReportHandlerError(ex, call_name);
  } catch (...) {
    ReportNonStdHandlerError(call_name);
  }
}

struct NoRequest final {};

template <typename RawResponder, typename Request>
struct CallData final {
  ::grpc::ServerContext context{};
  Request request{};
  RawResponder raw_responder{&context};
  AsyncMethodInvocation prepare{};
};

template <typename Handler, typename Stub, typename Request, typename Response>
void ListenAsync(
    ReactorData<Handler>& reactor, std::string_view call_name,
    RawResponseReaderPreparer<Stub, Request, Response> prepare_func,
    ResponseReaderHandlerFunc<Handler, Request, Response> handler_func) {
  auto call_data =
      std::make_unique<CallData<RawResponseWriter<Response>, Request>>();

  // this request for an incoming RPC has to be performed synchronously
  (reactor.GetStub().*prepare_func)(
      &call_data->context, &call_data->request, &call_data->raw_responder,
      &reactor.GetQueue(), &reactor.GetQueue(), call_data->prepare.GetTag());

  engine::CriticalAsyncNoSpan(
      reactor.GetHandlerTaskProcessor(),
      [&reactor, call_name, prepare_func, handler_func,
       call_data = std::move(call_data), token = reactor.GetWaitToken()] {
        if (!call_data->prepare.Wait()) {
          // the CompletionQueue is shutting down
          return;
        }

        // start a concurrent listener immediately, as advised by gRPC docs
        ListenAsync(reactor, call_name, prepare_func, handler_func);

        UnaryCall<Response> responder(call_data->context, call_name,
                                      call_data->raw_responder);
        WrapHandlerCall(call_name, reactor.GetHandler(), handler_func,
                        responder, std::move(call_data->request));
      })
      .Detach();
}

template <typename Handler, typename Stub, typename Request, typename Response>
void ListenAsync(ReactorData<Handler>& reactor, std::string_view call_name,
                 RawReaderPreparer<Stub, Request, Response> prepare_func,
                 ReaderHandlerFunc<Handler, Request, Response> handler_func) {
  auto call_data =
      std::make_unique<CallData<RawReader<Request, Response>, NoRequest>>();

  // this request for an incoming RPC has to be performed synchronously
  (reactor.GetStub().*prepare_func)(
      &call_data->context, &call_data->raw_responder, &reactor.GetQueue(),
      &reactor.GetQueue(), call_data->prepare.GetTag());

  engine::CriticalAsyncNoSpan(
      reactor.GetHandlerTaskProcessor(),
      [&reactor, call_name, prepare_func, handler_func,
       call_data = std::move(call_data), token = reactor.GetWaitToken()] {
        if (!call_data->prepare.Wait()) {
          // the CompletionQueue is shutting down
          return;
        }

        // start a concurrent listener immediately, as advised by gRPC docs
        ListenAsync(reactor, call_name, prepare_func, handler_func);

        InputStream<Request, Response> call(call_data->context, call_name,
                                            call_data->raw_responder);
        WrapHandlerCall(call_name, reactor.GetHandler(), handler_func, call);
      })
      .Detach();
}

template <typename Handler, typename Stub, typename Request, typename Response>
void ListenAsync(ReactorData<Handler>& reactor, std::string_view call_name,
                 RawWriterPreparer<Stub, Request, Response> prepare_func,
                 WriterHandlerFunc<Handler, Request, Response> handler_func) {
  auto call_data = std::make_unique<CallData<RawWriter<Response>, Request>>();

  // this request for an incoming RPC has to be performed synchronously
  (reactor.GetStub().*prepare_func)(
      &call_data->context, &call_data->request, &call_data->raw_responder,
      &reactor.GetQueue(), &reactor.GetQueue(), call_data->prepare.GetTag());

  engine::CriticalAsyncNoSpan(
      reactor.GetHandlerTaskProcessor(),
      [&reactor, call_name, prepare_func, handler_func,
       call_data = std::move(call_data), token = reactor.GetWaitToken()] {
        if (!call_data->prepare.Wait()) {
          // the CompletionQueue is shutting down
          return;
        }

        // start a concurrent listener immediately, as advised by gRPC docs
        ListenAsync(reactor, call_name, prepare_func, handler_func);

        OutputStream<Response> responder(call_data->context, call_name,
                                         call_data->raw_responder);
        WrapHandlerCall(call_name, reactor.GetHandler(), handler_func,
                        responder, std::move(call_data->request));
      })
      .Detach();
}

template <typename Handler, typename Stub, typename Request, typename Response>
void ListenAsync(
    ReactorData<Handler>& reactor, std::string_view call_name,
    RawReaderWriterPreparer<Stub, Request, Response> prepare_func,
    ReaderWriterHandlerFunc<Handler, Request, Response> handler_func) {
  auto call_data = std::make_unique<
      CallData<RawReaderWriter<Request, Response>, NoRequest>>();

  // this request for an incoming RPC has to be performed synchronously
  (reactor.GetStub().*prepare_func)(
      &call_data->context, &call_data->raw_responder, &reactor.GetQueue(),
      &reactor.GetQueue(), call_data->prepare.GetTag());

  engine::CriticalAsyncNoSpan(
      reactor.GetHandlerTaskProcessor(),
      [&reactor, call_name, prepare_func, handler_func,
       call_data = std::move(call_data), token = reactor.GetWaitToken()] {
        if (!call_data->prepare.Wait()) {
          // the CompletionQueue is shutting down
          return;
        }

        // start a concurrent listener immediately, as advised by gRPC docs
        ListenAsync(reactor, call_name, prepare_func, handler_func);

        BidirectionalStream<Request, Response> responder(
            call_data->context, call_name, call_data->raw_responder);
        WrapHandlerCall(call_name, reactor.GetHandler(), handler_func,
                        responder);
      })
      .Detach();
}

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
