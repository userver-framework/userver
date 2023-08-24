#include <userver/server/handlers/ping.hpp>

#include <userver/components/component_context.hpp>
#include <userver/server/handlers/exceptions.hpp>
#include <userver/yaml_config/schema.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers {

Ping::Ping(const components::ComponentConfig& config,
           const components::ComponentContext& component_context)
    : HttpHandlerBase(config, component_context),
      components_(component_context) {}

std::string Ping::HandleRequestThrow(
    const http::HttpRequest& /*request*/,
    request::RequestContext& /*context*/) const {
  if (components_.IsAnyComponentInFatalState()) {
    throw InternalServerError();
  }

  return {};
}
yaml_config::Schema Ping::GetStaticConfigSchema() {
  auto schema = HttpHandlerBase::GetStaticConfigSchema();
  schema.UpdateDescription("handler-ping config");
  return schema;
}

}  // namespace server::handlers

USERVER_NAMESPACE_END
