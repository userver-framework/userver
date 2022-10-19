#include <userver/server/handlers/on_log_rotate.hpp>

#include <userver/components/component_context.hpp>
#include <userver/formats/json/iterator.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/logging/component.hpp>
#include <userver/yaml_config/schema.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers {

OnLogRotate::OnLogRotate(const components::ComponentConfig& config,
                         const components::ComponentContext& context)
    : HttpHandlerBase(config, context, /*is_monitor = */ true),
      logging_component_(context.FindComponent<components::Logging>()) {}

std::string OnLogRotate::HandleRequestThrow(const http::HttpRequest& request,
                                            request::RequestContext&) const {
  LOG_DEBUG() << "Start OnLogRotate handling";
  if (request.GetMethod() != http::HttpMethod::kPost) {
    throw std::runtime_error("unsupported method: " + request.GetMethodStr());
  }

  logging_component_.TryReopenFiles();

  LOG_DEBUG() << "Success OnLogRotate handling";
  return {};
}

yaml_config::Schema OnLogRotate::GetStaticConfigSchema() {
  auto schema = HttpHandlerBase::GetStaticConfigSchema();
  schema.UpdateDescription("handler-on-log-rotate config");
  return schema;
}

}  // namespace server::handlers

USERVER_NAMESPACE_END
