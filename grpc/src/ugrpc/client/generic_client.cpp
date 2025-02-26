#include <userver/ugrpc/client/generic_client.hpp>

#include <grpcpp/generic/generic_stub.h>

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

client::ResponseFuture<grpc::ByteBuffer> GenericClient::AsyncUnaryCall(
    std::string_view call_name,
    const grpc::ByteBuffer& request,
    std::unique_ptr<grpc::ClientContext> context,
    const GenericOptions& generic_options
) const {
    auto method_name = utils::StrCat<grpc::string>("/", call_name);
    return {
        impl::CreateGenericCallParams(
            impl_, call_name, std::move(context), generic_options.qos, generic_options.metrics_call_name
        ),
        [&method_name](
            impl::ClientData::StubHandle& stub,
            grpc::ClientContext* context,
            const grpc::ByteBuffer& request,
            grpc::CompletionQueue* cq
        ) { return stub.Get<grpc::GenericStub>().PrepareUnaryCall(context, method_name, request, cq); },
        request,
    };
}

grpc::ByteBuffer GenericClient::UnaryCall(
    std::string_view call_name,
    const grpc::ByteBuffer& request,
    std::unique_ptr<grpc::ClientContext> context,
    const GenericOptions& generic_options
) const {
    return AsyncUnaryCall(call_name, request, std::move(context), generic_options).Get();
}

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
