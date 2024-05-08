#include <userver/storages/rocks/component.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

Rocks::Rocks(const components::ComponentConfig& config,
    const components::ComponentContext& context)
    : LoggableComponentBase(config, context), 
      task_processor_(context.GetTaskProcessor(config["task-processor"].As<std::string>())) {}

template <typename Client>
Client Rocks::MakeClient(const std::string& db_path) {
    return Client(db_path, task_processor_);
}

}  // namespace components

USERVER_NAMESPACE_END
