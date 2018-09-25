#include <server/handlers/handler_base.hpp>

#include <json_config/value.hpp>
#include <server/component.hpp>

namespace server {
namespace handlers {

HandlerBase::HandlerBase(const components::ComponentConfig& config,
                         const components::ComponentContext& component_context,
                         bool is_monitor)
    : config_(HandlerConfig::ParseFromJson(config.Json(), config.FullPath(),
                                           config.ConfigVarsPtr())),
      is_monitor_(is_monitor) {
  auto server_component = component_context.FindComponent<components::Server>();
  if (!server_component)
    throw std::runtime_error("can't find server component");

  is_enabled_ =
      json_config::ParseOptionalBool(config.Json(), "enabled",
                                     config.FullPath(), config.ConfigVarsPtr())
          .value_or(true);
  if (IsEnabled()) {
    if (!server_component->AddHandler(*this, component_context))
      throw std::runtime_error("can't add handler to server");
  }
}

const HandlerConfig& HandlerBase::GetConfig() const { return config_; }

}  // namespace handlers
}  // namespace server
