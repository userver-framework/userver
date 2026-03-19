#include <server/middlewares/graceful_shutdown_headers.hpp>

#include <fmt/format.h>
#include <fmt/ranges.h>

#include <dynamic_config/variables/GRACEFUL_SHUTDOWN_HEADERS.hpp>

#include <server/request/internal_request_context.hpp>
#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/server/request/request_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::middlewares {

GracefulShutdownHeaders::GracefulShutdownHeaders(const handlers::HttpHandlerBase&, const components::State& state)
    : HttpMiddlewareBase(),
      state_(state)
{}

void GracefulShutdownHeaders::HandleRequest(http::HttpRequest& request, request::RequestContext& context) const {
    if (state_.GetServiceLifetimeStage() == components::ServiceLifetimeStage::kGracefulShutdown) {
        const auto config = context.GetInternalContext().GetConfigSnapshot();
        auto graceful_shutdown_headers = config[::dynamic_config::GRACEFUL_SHUTDOWN_HEADERS];
        if (graceful_shutdown_headers.enabled) {
            auto& response = request.GetHttpResponse();
            for (const auto& [name, values] : graceful_shutdown_headers.headers.extra) {
                response.SetHeader(name, fmt::to_string(fmt::join(values, ", ")));
            }
        }
    }

    Next(request, context);
}

GracefulShutdownHeadersFactory::GracefulShutdownHeadersFactory(
    const components::ComponentConfig& config,
    const components::ComponentContext& context
)
    : HttpMiddlewareFactoryBase(config, context),
      state_(context)
{}

std::unique_ptr<HttpMiddlewareBase> GracefulShutdownHeadersFactory::Create(
    const handlers::HttpHandlerBase& handler,
    yaml_config::YamlConfig
) const {
    return std::make_unique<GracefulShutdownHeaders>(handler, state_);
}

}  // namespace server::middlewares

USERVER_NAMESPACE_END
