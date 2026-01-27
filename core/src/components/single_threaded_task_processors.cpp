#include <userver/components/single_threaded_task_processors.hpp>

#include <engine/task/task_processor_config.hpp>
#include <userver/components/component.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#ifndef ARCADIA_ROOT
#include "generated/src/components/single_threaded_task_processors.yaml.hpp"  // Y_IGNORE
#endif

USERVER_NAMESPACE_BEGIN

namespace components {

SingleThreadedTaskProcessors::SingleThreadedTaskProcessors(
    const ComponentConfig& config,
    const ComponentContext& context
)
    : ComponentBase(config, context),
      pool_(config.As<engine::TaskProcessorConfig>())
{}

yaml_config::Schema SingleThreadedTaskProcessors::GetStaticConfigSchema() {
    return yaml_config::MergeSchemasFromResource<ComponentBase>("src/components/single_threaded_task_processors.yaml");
}

SingleThreadedTaskProcessors::~SingleThreadedTaskProcessors() = default;

}  // namespace components

USERVER_NAMESPACE_END
