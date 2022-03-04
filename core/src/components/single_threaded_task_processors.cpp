#include <userver/components/single_threaded_task_processors.hpp>

#include <engine/task/task_processor_config.hpp>
#include <userver/components/component.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

SingleThreadedTaskProcessors::SingleThreadedTaskProcessors(
    const ComponentConfig& config, const ComponentContext& context)
    : LoggableComponentBase(config, context),
      pool_(config.As<engine::TaskProcessorConfig>()) {}

std::string SingleThreadedTaskProcessors::GetStaticConfigSchema() {
  return R"(
type: object
description: single-threaded-task-processors config
additionalProperties: false
properties: {}
)";
}

SingleThreadedTaskProcessors::~SingleThreadedTaskProcessors() = default;

}  // namespace components

USERVER_NAMESPACE_END
