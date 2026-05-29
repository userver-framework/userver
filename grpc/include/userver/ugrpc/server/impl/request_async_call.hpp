#pragma once

#include <grpcpp/completion_queue.h>
#include <grpcpp/server_context.h>
#include <grpcpp/support/async_stream.h>
#include <grpcpp/support/async_unary_call.h>

#include <userver/ugrpc/impl/async_service.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::impl {

template <typename GrpcppService, typename Request, typename Response>
void RequestAsyncCall(
    ugrpc::impl::AsyncService<GrpcppService>& async_service,
    int method_id,
    grpc::ServerContext* server_context,
    Request* request,
    grpc::ServerAsyncResponseWriter<Response>* stream,
    grpc::CompletionQueue* call_cq,
    grpc::ServerCompletionQueue* notification_cq,
    void* tag
) {
    async_service.RequestAsyncUnary(method_id, server_context, request, stream, call_cq, notification_cq, tag);
}

template <typename GrpcppService, typename Request, typename Response>
void RequestAsyncCall(
    ugrpc::impl::AsyncService<GrpcppService>& async_service,
    int method_id,
    grpc::ServerContext* server_context,
    grpc::ServerAsyncReader<Response, Request>* stream,
    grpc::CompletionQueue* call_cq,
    grpc::ServerCompletionQueue* notification_cq,
    void* tag
) {
    async_service.RequestAsyncClientStreaming(method_id, server_context, stream, call_cq, notification_cq, tag);
}

template <typename GrpcppService, typename Request, typename Response>
void RequestAsyncCall(
    ugrpc::impl::AsyncService<GrpcppService>& async_service,
    int method_id,
    grpc::ServerContext* server_context,
    Request* request,
    grpc::ServerAsyncWriter<Response>* stream,
    grpc::CompletionQueue* call_cq,
    grpc::ServerCompletionQueue* notification_cq,
    void* tag
) {
    async_service
        .RequestAsyncServerStreaming(method_id, server_context, request, stream, call_cq, notification_cq, tag);
}

template <typename GrpcppService, typename Request, typename Response>
void RequestAsyncCall(
    ugrpc::impl::AsyncService<GrpcppService>& async_service,
    int method_id,
    grpc::ServerContext* server_context,
    grpc::ServerAsyncReaderWriter<Response, Request>* stream,
    grpc::CompletionQueue* call_cq,
    grpc::ServerCompletionQueue* notification_cq,
    void* tag
) {
    async_service.RequestAsyncBidiStreaming(method_id, server_context, stream, call_cq, notification_cq, tag);
}

void RequestAsyncCall(
    ugrpc::impl::AsyncGenericService& async_generic_service,
    grpc::GenericServerContext* server_context,
    grpc::GenericServerAsyncReaderWriter* stream,
    grpc::CompletionQueue* call_cq,
    grpc::ServerCompletionQueue* notification_cq,
    void* tag
);

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
