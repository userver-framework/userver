#include <ugrpc/client/middlewares/origin/middleware.hpp>

#include <ugrpc/impl/rpc_metadata.hpp>
#include <userver/ugrpc/impl/to_string.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::middlewares::origin {

Middleware::Middleware(Settings&& settings)
    : settings_(std::move(settings))
{}

void Middleware::PreStartCall(MiddlewareCallContext& context) const {
    if (settings_.user_agent) {
        context.GetClientContext().AddMetadata(ugrpc::impl::kXOrigin, ugrpc::impl::ToGrpcString(*settings_.user_agent));
    }
}

}  // namespace ugrpc::client::middlewares::origin

USERVER_NAMESPACE_END
