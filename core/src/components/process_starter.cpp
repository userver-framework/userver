#include <components/process_starter.hpp>

namespace components {

ProcessStarter::ProcessStarter(const ComponentConfig& config,
                               const ComponentContext& context)
    : LoggableComponentBase(config, context),
      process_starter_(context.GetTaskProcessor(
          config["task_processor"].As<std::string>())) {}

}  // namespace components
