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
  yaml_config::Schema schema(R"(
type: object
description: process-starter
additionalProperties: false
properties: {}
)");
  yaml_config::Merge(schema, LoggableComponentBase::GetStaticConfigSchema());
  return schema;
}

}  // namespace components

USERVER_NAMESPACE_END
