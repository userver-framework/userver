#include <userver/components/single_threaded_task_processors.hpp>

#include <engine/task/task_processor_config.hpp>
#include <userver/components/component.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

SingleThreadedTaskProcessors::SingleThreadedTaskProcessors(
    const ComponentConfig& config, const ComponentContext& context)
    : LoggableComponentBase(config, context),
      pool_(config.As<engine::TaskProcessorConfig>()) {}

yaml_config::Schema SingleThreadedTaskProcessors::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<LoggableComponentBase>(R"(
type: object
description: single-threaded-task-processors config
additionalProperties: false
properties:
    guess-cpu-limit:
        type: boolean
        description: .
        defaultDescription: false
    worker_threads:
        type: integer
        description: .
        defaultDescription: 6
    thread_name:
        type: string
        description: .
    os-scheduling:
        type: string
        description: |
            OS scheduling mode for the task processor threads.
            `idle` sets the lowest pririty.
            `low-priority` sets the priority below `normal` but
            higher than `idle`.   
        defaultDescription: normal
        enum:
          - normal
          - low-priority
          - idle
    task-trace:
        type: object
        description: .
        additionalProperties: false
        properties:
            every:
                type: integer
                description: .
                defaultDescription: 1000
            max-context-switch-count:
                type: integer
                description: .
                defaultDescription: 0
            logger:
                type: string
                description: .
)");
}

SingleThreadedTaskProcessors::~SingleThreadedTaskProcessors() = default;

}  // namespace components

USERVER_NAMESPACE_END
