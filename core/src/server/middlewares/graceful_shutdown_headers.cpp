#include <server/middlewares/graceful_shutdown_headers.hpp>

#include <fmt/format.h>
#include <fmt/ranges.h>

#include <dynamic_config/variables/GRACEFUL_SHUTDOWN_HEADERS.hpp>
#include <server/request/internal_request_context.hpp>
#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/dynamic_config/storage/component.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/server/request/request_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::middlewares {

GracefulShutdownHeaders::GracefulShutdownHeaders(
    const handlers::HttpHandlerBase&,
    const components::State& state,
    const dynamic_config::Source& config_source
)
    : HttpMiddlewareBase(),
      state_(state),
      config_source_(config_source)
{}

void GracefulShutdownHeaders::HandleRequest(http::HttpRequest& request, request::RequestContext& context) const {
    Next(request, context);

    if (state_.GetServiceLifetimeStage() == components::ServiceLifetimeStage::kGracefulShutdown) {
        const auto config = config_source_.GetSnapshot();
        auto graceful_shutdown_headers = config[::dynamic_config::GRACEFUL_SHUTDOWN_HEADERS];
        if (graceful_shutdown_headers.enabled) {
            auto& response = request.GetHttpResponse();
            for (const auto& [name, values] : graceful_shutdown_headers.headers.extra) {
                response.SetHeader(name, fmt::to_string(fmt::join(values, ", ")));
            }
        }
    }
}

GracefulShutdownHeadersFactory::GracefulShutdownHeadersFactory(
    const components::ComponentConfig& config,
    const components::ComponentContext& context
)
    : HttpMiddlewareFactoryBase(config, context),
      state_(context),
      config_source_(context.FindComponent<components::DynamicConfig>().GetSource())
{}

std::unique_ptr<HttpMiddlewareBase> GracefulShutdownHeadersFactory::Create(
    const handlers::HttpHandlerBase& handler,
    yaml_config::YamlConfig
) const {
    return std::make_unique<GracefulShutdownHeaders>(handler, state_, config_source_);
}

}  // namespace server::middlewares

USERVER_NAMESPACE_END
