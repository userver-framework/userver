#include <userver/server/handlers/handler_base.hpp>

#include <server/server_config.hpp>
#include <userver/components/component.hpp>
#include <userver/server/component.hpp>
#include <userver/server/handlers/handler_config.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#ifndef ARCADIA_ROOT
#include "generated/src/server/handlers/handler_base.yaml.hpp"  // Y_IGNORE
#endif

USERVER_NAMESPACE_BEGIN

namespace server::handlers {

HandlerBase::HandlerBase(
    const components::ComponentConfig& config,
    const components::ComponentContext& context,
    bool is_monitor
)
    : ComponentBase(config, context),
      is_monitor_(config["monitor-handler"].As<bool>(is_monitor)),
      config_(ParseHandlerConfigsWithDefaults(
          config,
          context.FindComponent<components::Server>().GetServer().GetConfig(),
          is_monitor_
      ))
{}

const HandlerConfig& HandlerBase::GetConfig() const { return config_; }

yaml_config::Schema HandlerBase::GetStaticConfigSchema() {
    return yaml_config::MergeSchemasFromResource<ComponentBase>("src/server/handlers/handler_base.yaml");
}

}  // namespace server::handlers

USERVER_NAMESPACE_END
