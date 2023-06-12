#include <userver/ugrpc/client/client_factory_component.hpp>

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/dynamic_config/storage/component.hpp>
#include <userver/testsuite/testsuite_support.hpp>
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
  const auto config_source =
      context.FindComponent<components::DynamicConfig>().GetSource();

  auto& testsuite_grpc =
      context.FindComponent<components::TestsuiteSupport>().GetGrpcControl();

  MiddlewareFactories mws;
  auto middleware_names =
      config["middlewares"].As<std::vector<std::string>>({});
  for (const auto& name : middleware_names) {
    auto& component = context.FindComponent<MiddlewareComponentBase>(name);
    mws.push_back(component.GetMiddlewareFactory());
  }
  factory_.emplace(config.As<ClientFactoryConfig>(), task_processor, mws,
                   *queue, statistics_storage, testsuite_grpc, config_source);
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
            description: value of channel argument, must be string or integer
        properties: {}
    native-log-level:
        type: string
        description: min log level for the native gRPC library
        defaultDescription: error
        enum:
          - debug
          - info
          - error
    auth-type:
        type: string
        description: an optional authentication method
        defaultDescription: insecure
        enum:
          - insecure
          - ssl
    default-service-config:
        type: string
        description: |
            Default value for gRPC `service config`. See
            https://github.com/grpc/grpc/blob/master/doc/service_config.md
            This value is used if the name resolution process can't get value
            from DNS
        defaultDescription: absent
    channel-count:
        type: integer
        description: |
            Number of channels created for each endpoint.
        defaultDescription: 1
    middlewares:
        type: array
        items:
            type: string
            description: middleware name
        description: middlewares names
        defaultDescription: '[]'
)");
}

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
