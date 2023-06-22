#include <userver/ugrpc/server/server_component.hpp>

#include <userver/components/component.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/dynamic_config/storage/component.hpp>
#include <userver/logging/component.hpp>
#include <userver/logging/fwd.hpp>
#include <userver/logging/null_logger.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server {

namespace {

logging::LoggerPtr GetLogger(const components::ComponentContext& context,
                             const ServerConfig config) {
  if (config.access_log_logger_name.empty()) return logging::MakeNullLogger();

  return context.FindComponent<components::Logging>().GetLogger(
      config.access_log_logger_name);
}

}  // namespace

ServerComponent::ServerComponent(const components::ComponentConfig& config,
                                 const components::ComponentContext& context)
    : LoggableComponentBase(config, context),
      config_(config.As<ServerConfig>()),
      server_(
          config_,
          context.FindComponent<components::StatisticsStorage>().GetStorage(),
          GetLogger(context, config_),
          context.FindComponent<components::DynamicConfig>().GetSource()) {}

Server& ServerComponent::GetServer() noexcept { return server_; }

void ServerComponent::OnAllComponentsLoaded() { server_.Start(); }

void ServerComponent::OnAllComponentsAreStopping() { server_.Stop(); }

yaml_config::Schema ServerComponent::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<LoggableComponentBase>(R"(
type: object
description: Component that configures and manages the gRPC server.
additionalProperties: false
properties:
    port:
        type: integer
        description: the port to use for all gRPC services, or 0 to pick any available
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
)");
}

}  // namespace ugrpc::server

USERVER_NAMESPACE_END
