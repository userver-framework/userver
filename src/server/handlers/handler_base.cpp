#include <server/handlers/handler_base.hpp>

#include <json_config/value.hpp>

namespace server {
namespace handlers {

HandlerBase::HandlerBase(
    const components::ComponentConfig& config,
    const components::ComponentContext& /*component_context*/, bool is_monitor)
    : config_(HandlerConfig::ParseFromJson(config.Json(), config.FullPath(),
                                           config.ConfigVarsPtr())),
      is_monitor_(is_monitor) {
  is_enabled_ =
      json_config::ParseOptionalBool(config.Json(), "enabled",
                                     config.FullPath(), config.ConfigVarsPtr())
          .value_or(true);
}

const HandlerConfig& HandlerBase::GetConfig() const { return config_; }

}  // namespace handlers
}  // namespace server
