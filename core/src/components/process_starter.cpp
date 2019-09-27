#include <components/process_starter.hpp>

namespace components {

ProcessStarter::ProcessStarter(const ComponentConfig& config,
                               const ComponentContext& context)
    : LoggableComponentBase(config, context),
      process_starter_(context.GetTaskProcessor(
          config.Parse<std::string>("task_processor"))) {}

}  // namespace components
