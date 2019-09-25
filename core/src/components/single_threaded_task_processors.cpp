#include <components/single_threaded_task_processors.hpp>

#include <components/component_config.hpp>
#include <engine/task/task_processor_config.hpp>

namespace components {

namespace {

engine::TaskProcessorConfig GetConfig(const ComponentConfig& config) {
  return engine::TaskProcessorConfig::ParseFromYaml(
      config.Yaml(), config.FullPath(), config.ConfigVarsPtr());
}

}  // namespace

SingleThreadedTaskProcessors::SingleThreadedTaskProcessors(
    const ComponentConfig& config, const ComponentContext& context)
    : LoggableComponentBase(config, context), pool_(GetConfig(config)) {}

SingleThreadedTaskProcessors::~SingleThreadedTaskProcessors() = default;

}  // namespace components
