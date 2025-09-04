#include <userver/ugrpc/client/generic_client.hpp>

#include <utility>

#include <grpcpp/generic/generic_stub.h>

#include <userver/ugrpc/client/impl/call_params.hpp>
#include <userver/ugrpc/client/impl/perform_unary_call.hpp>
#include <userver/utils/algo.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

namespace {

struct GenericService final {
    using Stub = grpc::GenericStub;
};

}  // namespace

GenericClient::GenericClient(impl::ClientInternals&& internals)
    : impl_(std::move(internals), impl::GenericClientTag{}, std::in_place_type<GenericService>) {
    // There is no technical reason why QOS configs should be unsupported here.
    // However, it would be difficult to detect non-existent RPC names in QOS.
    UINVARIANT(!impl_.GetClientQos(), "Client QOS configs are unsupported for generic services");
}

ResponseFuture<grpc::ByteBuffer> GenericClient::AsyncUnaryCall(
    std::string_view call_name,
    const grpc::ByteBuffer& request,
    CallOptions call_options,
    GenericOptions generic_options
) const {
    auto method_name = utils::StrCat<grpc::string>("/", call_name);
    return {
        impl::CreateGenericCallParams(impl_, call_name, std::move(call_options), std::move(generic_options)),
        impl::PrepareUnaryCallProxy(&grpc::GenericStub::PrepareUnaryCall, std::move(method_name)),
        request,
    };
}

grpc::ByteBuffer GenericClient::UnaryCall(
    std::string_view call_name,
    const grpc::ByteBuffer& request,
    CallOptions call_options,
    GenericOptions generic_options
) const {
    auto method_name = utils::StrCat<grpc::string>("/", call_name);
    return impl::PerformUnaryCall(
        impl::CreateGenericCallParams(impl_, call_name, std::move(call_options), std::move(generic_options)),
        impl::PrepareUnaryCallProxy(&grpc::GenericStub::PrepareUnaryCall, std::move(method_name)),
        request
    );
}

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
