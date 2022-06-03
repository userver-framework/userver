#include <userver/ugrpc/client/client_factory_component.hpp>

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <userver/ugrpc/server/server_component.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

ClientFactoryComponent::ClientFactoryComponent(
    const components::ComponentConfig& config,
    const components::ComponentContext& context)
    : LoggableComponentBase(config, context) {
  auto& task_processor =
      context.GetTaskProcessor(config["task-processor"].As<std::string>());

  grpc::CompletionQueue* queue = nullptr;
  if (auto* const server =
          context.FindComponentOptional<ugrpc::server::ServerComponent>()) {
    queue = &server->GetServer().GetCompletionQueue();
  } else {
    queue_.emplace();
    queue = &queue_->GetQueue();
  }

  auto& statistics_storage =
      context.FindComponent<components::StatisticsStorage>().GetStorage();

  factory_.emplace(config.As<ClientFactoryConfig>(), task_processor, *queue,
                   statistics_storage);
}

ClientFactory& ClientFactoryComponent::GetFactory() { return *factory_; }

yaml_config::Schema ClientFactoryComponent::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<components::LoggableComponentBase>(R"(
type: object
description: Provides a ClientFactory in the component system
additionalProperties: false
properties:
    task-processor:
        type: string
        description: the task processor for blocking channel creation
    channel-args:
        type: object
        description: a map of channel arguments, see gRPC Core docs
        defaultDescription: '{}'
        additionalProperties:
            type: string
            description: value of chanel argument, must be string or integer
        properties: {}
)");
}

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
