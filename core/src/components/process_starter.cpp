#include <userver/components/process_starter.hpp>

#include <userver/components/component.hpp>
#include <userver/engine/task/task_base.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#ifndef ARCADIA_ROOT
#include "generated/src/components/process_starter.yaml.hpp"  // Y_IGNORE
#endif

USERVER_NAMESPACE_BEGIN

namespace components {

namespace {

engine::TaskProcessor& GetTaskProcessorOrDefault(
    const std::optional<std::string>& task_processor_name,
    const ComponentContext& context
) {
    return task_processor_name
               ? context.GetTaskProcessor(*task_processor_name)
               : engine::current_task::GetTaskProcessor();
}

}  // namespace

ProcessStarter::ProcessStarter(const ComponentConfig& config, const ComponentContext& context)
    : ComponentBase(config, context),
      process_starter_(GetTaskProcessorOrDefault(config["task_processor"].As<std::optional<std::string>>(), context))
{}

yaml_config::Schema ProcessStarter::GetStaticConfigSchema() {
    return yaml_config::MergeSchemasFromResource<ComponentBase>("src/components/process_starter.yaml");
}

}  // namespace components

USERVER_NAMESPACE_END
