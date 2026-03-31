#pragma once

#include <userver/ugrpc/impl/async_service.hpp>
#include <userver/ugrpc/server/impl/call_traits.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::impl {

template <typename CallTraits, typename Service>
void RequestAsyncCall(
    ugrpc::impl::AsyncService<Service>& async_service,
    int method_id,
    grpc::ServerContext& server_context,
    typename CallTraits::InitialRequest& initial_request,
    typename CallTraits::RawResponder& stream,
    grpc::CompletionQueue& call_cq,
    grpc::ServerCompletionQueue& notification_cq,
    void* tag
) {
    if constexpr (CallTraits::kRpcType == RpcType::kUnary) {
        async_service
            .RequestAsyncUnary(method_id, &server_context, &initial_request, &stream, &call_cq, &notification_cq, tag);
    } else if constexpr (CallTraits::kRpcType == RpcType::kClientStreaming) {
        async_service.RequestAsyncClientStreaming(method_id, &server_context, &stream, &call_cq, &notification_cq, tag);
    } else if constexpr (CallTraits::kRpcType == RpcType::kServerStreaming) {
        async_service.RequestAsyncServerStreaming(
            method_id,
            &server_context,
            &initial_request,
            &stream,
            &call_cq,
            &notification_cq,
            tag
        );
    } else if constexpr (CallTraits::kRpcType == RpcType::kBidiStreaming) {
        async_service.RequestAsyncBidiStreaming(method_id, &server_context, &stream, &call_cq, &notification_cq, tag);
    } else {
        static_assert(!sizeof(CallTraits), "Invalid kCallCategory");
    }
}

template <typename CallTraits>
void RequestAsyncCall(
    ugrpc::impl::AsyncGenericService& async_generic_service,
    int /*method_id*/,
    grpc::GenericServerContext& server_context,
    NoInitialRequest& /*initial_request*/,
    grpc::GenericServerAsyncReaderWriter& stream,
    grpc::CompletionQueue& call_cq,
    grpc::ServerCompletionQueue& notification_cq,
    void* tag
) {
    async_generic_service.GetAsyncGenericService()
        .RequestCall(&server_context, &stream, &call_cq, &notification_cq, tag);
}

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
