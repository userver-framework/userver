#include <userver/components/single_threaded_task_processors.hpp>

#include <engine/task/task_processor_config.hpp>
#include <userver/components/component_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

SingleThreadedTaskProcessors::SingleThreadedTaskProcessors(
    const ComponentConfig& config, const ComponentContext& context)
    : LoggableComponentBase(config, context),
      pool_(config.As<engine::TaskProcessorConfig>()) {}

SingleThreadedTaskProcessors::~SingleThreadedTaskProcessors() = default;

}  // namespace components

USERVER_NAMESPACE_END
