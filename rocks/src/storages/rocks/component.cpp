#include <userver/storages/rocks/component.hpp>

#include <memory>

#include <userver/storages/rocks/client.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

Rocks::Rocks(const components::ComponentConfig& config, const components::ComponentContext& context)
    : LoggableComponentBase(config, context),
      client_ptr_(std::make_shared<storages::rocks::Client>(config["db-path"].As<std::string>(), 
                  context.GetTaskProcessor(config["task-processor"].As<std::string>()))) {}

storages::rocks::ClientPtr Rocks::MakeClient() {
    return client_ptr_;
}

yaml_config::Schema Rocks::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<LoggableComponentBase>(R"(
type: object
description: Rocks client component
additionalProperties: false
properties:
        task-processor:
            type: string
            description: name of the task processor to run the blocking file write operations
        db-path:
            type: string
            description: name of path to rocksDb
)");
}
}  // namespace components

USERVER_NAMESPACE_END
