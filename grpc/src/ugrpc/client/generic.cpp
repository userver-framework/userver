#include <userver/ugrpc/client/generic.hpp>

#include <grpcpp/generic/generic_stub.h>

#include <userver/utils/algo.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

namespace {

struct GenericStubService final {
    using Stub = grpc::GenericStub;

    static std::unique_ptr<Stub> NewStub(std::shared_ptr<grpc::Channel> channel) {
        return std::make_unique<Stub>(channel);
    }
};

}  // namespace

GenericClient::GenericClient(impl::ClientDependencies&& client_params)
    : impl_(std::move(client_params), impl::GenericClientTag{}, std::in_place_type<GenericStubService>) {
    // There is no technical reason why QOS configs should be unsupported here.
    // However, it would be difficult to detect non-existent RPC names in QOS.
    UINVARIANT(!impl_.GetClientQos(), "Client QOS configs are unsupported for generic services");
}

client::UnaryCall<grpc::ByteBuffer> GenericClient::UnaryCall(
    std::string_view call_name,
    const grpc::ByteBuffer& request,
    std::unique_ptr<grpc::ClientContext> context,
    const GenericOptions& generic_options
) const {
    auto& stub = impl_.NextGenericStub<GenericStubService>();
    auto grpcpp_call_name = utils::StrCat<grpc::string>("/", call_name);
    return {
        impl::CreateGenericCallParams(
            impl_, call_name, std::move(context), generic_options.qos, generic_options.metrics_call_name
        ),
        [&stub, &grpcpp_call_name](
            grpc::ClientContext* context, const grpc::ByteBuffer& request, grpc::CompletionQueue* cq
        ) { return stub.PrepareUnaryCall(context, grpcpp_call_name, request, cq); },
        request,
    };
}

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
