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
    engine::TaskProcessor* task_processor =
        component_context.GetTaskProcessor(config_.task_processor);
    if (task_processor == nullptr) {
      throw std::runtime_error("can't find task_processor with name '" +
                               config_.task_processor + '\'');
    }
    try {
      server_component->AddHandler(*this, *task_processor);
    } catch (const std::exception& ex) {
      throw std::runtime_error(std::string("can't add handler to server: ") +
                               ex.what());
    }
  }
}

const HandlerConfig& HandlerBase::GetConfig() const { return config_; }

}  // namespace handlers
}  // namespace server
