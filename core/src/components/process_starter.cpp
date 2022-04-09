#include <userver/components/process_starter.hpp>

#include <userver/components/component.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

ProcessStarter::ProcessStarter(const ComponentConfig& config,
                               const ComponentContext& context)
    : LoggableComponentBase(config, context),
      process_starter_(context.GetTaskProcessor(
          config["task_processor"].As<std::string>())) {}

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
