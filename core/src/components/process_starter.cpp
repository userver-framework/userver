#include <userver/components/process_starter.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

ProcessStarter::ProcessStarter(const ComponentConfig& config,
                               const ComponentContext& context)
    : LoggableComponentBase(config, context),
      process_starter_(context.GetTaskProcessor(
          config["task_processor"].As<std::string>())) {}

}  // namespace components

USERVER_NAMESPACE_END
