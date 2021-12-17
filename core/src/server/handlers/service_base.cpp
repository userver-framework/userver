#include <userver/server/handlers/handler_base.hpp>

#include <userver/components/component.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers {

HandlerBase::HandlerBase(const components::ComponentConfig& config,
                         const components::ComponentContext& context,
                         bool is_monitor)
    : LoggableComponentBase(config, context),
      config_(config.As<HandlerConfig>()),
      is_monitor_(is_monitor) {}

const HandlerConfig& HandlerBase::GetConfig() const { return config_; }

}  // namespace server::handlers

USERVER_NAMESPACE_END
