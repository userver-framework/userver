#include <ugrpc/server/middlewares/graceful_shutdown_headers/middleware.hpp>

#include <dynamic_config/variables/GRACEFUL_SHUTDOWN_HEADERS.hpp>
#include <userver/ugrpc/impl/to_string.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::middlewares::graceful_shutdown_headers {

Middleware::Middleware(const components::State& state, dynamic_config::Source source)
    : state_(state),
      source_(std::move(source))
{}

void Middleware::OnCallStart(MiddlewareCallContext& context) const {
    if (state_.GetServiceLifetimeStage() != components::ServiceLifetimeStage::kGracefulShutdown) {
        return;
    }
    const auto& config = context.GetInitialDynamicConfig();
    const auto& graceful_shutdown_headers = config[::dynamic_config::GRACEFUL_SHUTDOWN_HEADERS];
    if (!graceful_shutdown_headers.enabled) {
        return;
    }
    auto& server_context = context.GetServerContext();
    for (const auto& [name, values] : graceful_shutdown_headers.headers.extra) {
        auto header_name = ugrpc::impl::ToGrpcString(name);
        for (const auto& value : values) {
            server_context.AddInitialMetadata(header_name, ugrpc::impl::ToGrpcString(value));
        }
    }
}

}  // namespace ugrpc::server::middlewares::graceful_shutdown_headers

USERVER_NAMESPACE_END
