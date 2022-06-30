#include <userver/os_signals/component.hpp>

#include <components/manager_config.hpp>
#include <userver/components/component_config.hpp>
#include <userver/components/manager.hpp>
#include <userver/yaml_config/merge_schemas.hpp>
#include <utils/internal_tag.hpp>

USERVER_NAMESPACE_BEGIN

namespace os_signals {

ProcessorComponent::ProcessorComponent(
    const components::ComponentConfig&,
    const components::ComponentContext& context)
    : manager_(context.GetTaskProcessor(
          context.GetManager().GetConfig().default_task_processor)) {}

os_signals::Processor& ProcessorComponent::Get() { return manager_; }

}  // namespace os_signals

USERVER_NAMESPACE_END
