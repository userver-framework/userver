#include <server/handlers/handler_base.hpp>

namespace server::handlers {

HandlerBase::HandlerBase(const components::ComponentConfig& config,
                         const components::ComponentContext& context,
                         bool is_monitor)
    : LoggableComponentBase(config, context),
      config_(config.As<HandlerConfig>()),
      is_monitor_(is_monitor),
      is_enabled_(config["enabled"].As<bool>(true)) {}

const HandlerConfig& HandlerBase::GetConfig() const { return config_; }

}  // namespace server::handlers
