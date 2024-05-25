#include <userver/components/process_starter.hpp>

#include <userver/components/component.hpp>
#include <userver/engine/task/task_base.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

namespace {

engine::TaskProcessor& GetTaskProcessorOrDefault(
    const std::optional<std::string>& task_processor_name,
    const ComponentContext& context) {
  return task_processor_name ? context.GetTaskProcessor(*task_processor_name)
                             : engine::current_task::GetTaskProcessor();
}

}  // namespace

ProcessStarter::ProcessStarter(const ComponentConfig& config,
                               const ComponentContext& context)
    : LoggableComponentBase(config, context),
      process_starter_(GetTaskProcessorOrDefault(
          config["task_processor"].As<std::optional<std::string>>(), context)) {
}

yaml_config::Schema ProcessStarter::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<LoggableComponentBase>(R"(
type: object
description: process-starter
additionalProperties: false
properties:
    task_processor:
        type: string
        description: the name of the TaskProcessor for process starting
)");
}

}  // namespace components

USERVER_NAMESPACE_END
