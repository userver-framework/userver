#include <userver/ugrpc/client/middlewares/testsuite/middleware.hpp>

#include <ugrpc/impl/rpc_metadata.hpp>
#include <userver/ugrpc/client/exceptions.hpp>
#include <userver/ugrpc/client/middlewares/base.hpp>
#include <userver/ugrpc/impl/to_string.hpp>
#include <userver/utils/algo.hpp>
#include <userver/utils/text_light.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::middlewares::testsuite {

namespace {

constexpr std::string_view kClientSuffix = "-client";

std::string_view RemoveClientSuffix(std::string_view client_name) {
    if (utils::text::EndsWith(client_name, kClientSuffix)) {
        client_name.remove_suffix(kClientSuffix.size());
    }
    return client_name;
}

}  // namespace

Middleware::Middleware(std::string_view client_name) : client_name_(RemoveClientSuffix(client_name)) {}

void Middleware::PreStartCall(MiddlewareCallContext& context) const {
    // This metadata allows testsuite mock handlers to distinguish between clients to different deploy units
    // for a single proto service.
    context.GetClientContext().AddMetadata(ugrpc::impl::kXTestsuiteClientName, ugrpc::impl::ToGrpcString(client_name_));
}

void Middleware::PostFinish(MiddlewareCallContext& context, const grpc::Status&) const {
    const auto& metadata = context.GetClientContext().GetServerTrailingMetadata();

    if (auto error_code = utils::FindOptional(metadata, ugrpc::impl::kXTestsuiteErrorCode)) {
        throw RpcInterruptedError(
            context.GetCallName(), fmt::format("Testsuite {}", ugrpc::impl::ToString(error_code.value()))
        );
    }
}

}  // namespace ugrpc::client::middlewares::testsuite

USERVER_NAMESPACE_END
