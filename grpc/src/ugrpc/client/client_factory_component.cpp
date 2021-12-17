#include <userver/ugrpc/client/client_factory_component.hpp>

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>

#include <userver/ugrpc/server/server_component.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

ClientFactoryComponent::ClientFactoryComponent(
    const components::ComponentConfig& config,
    const components::ComponentContext& context)
    : LoggableComponentBase(config, context) {
  auto& task_processor =
      context.GetTaskProcessor(config["task-processor"].As<std::string>());

  if (auto* server =
          context.FindComponentOptional<ugrpc::server::ServerComponent>()) {
    factory_.emplace(task_processor, server->GetServer().GetCompletionQueue());
  } else {
    queue_.emplace();
    factory_.emplace(task_processor, queue_->GetQueue());
  }
}

ClientFactory& ClientFactoryComponent::GetFactory() { return *factory_; }

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
