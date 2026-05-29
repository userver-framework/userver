#include "userver/ugrpc/server/impl/request_async_call.hpp"

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::impl {

void RequestAsyncCall(
    ugrpc::impl::AsyncGenericService& async_generic_service,
    grpc::GenericServerContext* server_context,
    grpc::GenericServerAsyncReaderWriter* stream,
    grpc::CompletionQueue* call_cq,
    grpc::ServerCompletionQueue* notification_cq,
    void* tag
) {
    async_generic_service.GetAsyncGenericService().RequestCall(server_context, stream, call_cq, notification_cq, tag);
}

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
