#include <userver/components/component_fwd.hpp>

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/engine/task/current_task.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

std::string_view GetCurrentComponentName(const ComponentContext& context) { return context.GetComponentName(); }

engine::TaskProcessor& GetFsTaskProcessor(const ComponentConfig& config, const ComponentContext& context) {
    return config["fs-task-processor"].IsMissing()
               ? engine::current_task::GetBlockingTaskProcessor()
               : context.GetTaskProcessor(config["fs-task-processor"].As<std::string>());
}

}  // namespace components

USERVER_NAMESPACE_END
