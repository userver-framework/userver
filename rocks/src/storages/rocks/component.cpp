#include <userver/storages/rocks/component.hpp>

#include <memory>

#include <userver/storages/rocks/client.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::rocks {

Component::Component(const components::ComponentConfig& config,
                     const components::ComponentContext& context)
    : LoggableComponentBase(config, context),
      client_ptr_(std::make_shared<storages::rocks::Client>(
          config["db-path"].As<std::string>(),
          context.GetTaskProcessor(
              config["task-processor"].As<std::string>()))) {}

storages::rocks::ClientPtr Component::MakeClient() { return client_ptr_; }

yaml_config::Schema Component::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<LoggableComponentBase>(R"(
type: object
description: Rocks client component
additionalProperties: false
properties:
    task-processor:
        type: string
        description: name of the task processor to run the blocking file operations
    db-path:
        type: string
        description: path to database file
)");
}
}  // namespace storages::rocks

USERVER_NAMESPACE_END
