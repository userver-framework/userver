#include <components/single_threaded_task_processors.hpp>

#include <components/component_config.hpp>
#include <engine/task/task_processor_config.hpp>

namespace components {

SingleThreadedTaskProcessors::SingleThreadedTaskProcessors(
    const ComponentConfig& config, const ComponentContext& context)
    : LoggableComponentBase(config, context),
      pool_(config.As<engine::TaskProcessorConfig>()) {}

SingleThreadedTaskProcessors::~SingleThreadedTaskProcessors() = default;

}  // namespace components
