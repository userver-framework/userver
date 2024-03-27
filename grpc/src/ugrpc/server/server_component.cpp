#include <userver/ugrpc/server/server_component.hpp>

#include <userver/components/component.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/dynamic_config/storage/component.hpp>
#include <userver/logging/component.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <ugrpc/server/impl/parse_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server {

ServerComponent::ServerComponent(const components::ComponentConfig& config,
                                 const components::ComponentContext& context)
    : LoggableComponentBase(config, context),
      server_(
          impl::ParseServerConfig(config, context),
          context.FindComponent<components::StatisticsStorage>().GetStorage(),
          context.FindComponent<components::DynamicConfig>().GetSource()),
      service_defaults_(std::make_unique<impl::ServiceDefaults>(
          impl::ParseServiceDefaults(config["service-defaults"], context))) {}

ServerComponent::~ServerComponent() { server_.Stop(); }

Server& ServerComponent::GetServer() noexcept { return server_; }

ServiceConfig ServerComponent::ParseServiceConfig(
    const components::ComponentConfig& config,
    const components::ComponentContext& context) {
  return impl::ParseServiceConfig(config, context, *service_defaults_);
}

void ServerComponent::OnAllComponentsLoaded() { server_.Start(); }

void ServerComponent::OnAllComponentsAreStopping() { server_.StopServing(); }

yaml_config::Schema ServerComponent::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<LoggableComponentBase>(R"(
type: object
description: Component that configures and manages the gRPC server.
additionalProperties: false
properties:
    port:
        type: integer
        description: the port to use for all gRPC services, or 0 to pick any available
    unix-socket-path:
        type: string
        description: unix socket absolute path
    completion-queue-count:
        type: integer
        description: |
            completion queue count to create. Should be ~2 times less than worker
            threads for best RPS.
        minimum: 1
    channel-args:
        type: object
        description: a map of channel arguments, see gRPC Core docs
        defaultDescription: '{}'
        additionalProperties:
            type: string
            description: value of channel argument, must be string or integer
        properties: {}
    access-tskv-logger:
        type: string
        description: name of 'access-tskv.log' logger
    native-log-level:
        type: string
        description: min log level for the native gRPC library
        defaultDescription: 'error'
        enum:
          - trace
          - debug
          - info
          - warning
          - error
          - critical
          - none
    enable-channelz:
        type: boolean
        description: enable channelz
    service-defaults:
        type: object
        description: omitted options for service components will default to the corresponding option from here
        additionalProperties: false
        properties:
            task-processor:
                type: string
                description: the task processor to use for responses
            middlewares:
                type: array
                description: middlewares names to use
                items:
                    type: string
                    description: middleware component name
)");
}

}  // namespace ugrpc::server

USERVER_NAMESPACE_END
