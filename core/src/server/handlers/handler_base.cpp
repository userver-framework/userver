#include <server/handlers/handler_base.hpp>

#include <yaml_config/value.hpp>

namespace server::handlers {

HandlerBase::HandlerBase(const components::ComponentConfig& config,
                         const components::ComponentContext& context,
                         bool is_monitor)
    : LoggableComponentBase(config, context),
      config_(HandlerConfig::ParseFromYaml(config.Yaml(), config.FullPath(),
                                           config.ConfigVarsPtr())),
      is_monitor_(is_monitor) {
  is_enabled_ =
      yaml_config::ParseOptionalBool(config.Yaml(), "enabled",
                                     config.FullPath(), config.ConfigVarsPtr())
          .value_or(true);
}

const HandlerConfig& HandlerBase::GetConfig() const { return config_; }

}  // namespace server::handlers
