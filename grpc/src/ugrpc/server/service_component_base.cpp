#include <userver/ugrpc/server/service_component_base.hpp>

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/utils/assert.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <ugrpc/server/impl/parse_config.hpp>
#include <userver/ugrpc/server/middlewares/base.hpp>
#include <userver/ugrpc/server/server_component.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server {

ServiceComponentBase::ServiceComponentBase(
    const components::ComponentConfig& config,
    const components::ComponentContext& context)
    : LoggableComponentBase(config, context),
      server_(context.FindComponent<ServerComponent>()),
      config_(server_.ParseServiceConfig(config, context)) {}

void ServiceComponentBase::RegisterService(ServiceBase& service) {
  UINVARIANT(!registered_.exchange(true), "Register must only be called once");
  server_.GetServer().AddService(service, std::move(config_));
}

yaml_config::Schema ServiceComponentBase::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<components::LoggableComponentBase>(R"(
type: object
description: base class for all the gRPC service components
additionalProperties: false
properties:
    task-processor:
        type: string
        description: the task processor to use for responses
        defaultDescription: uses grpc-server.service-defaults.task-processor
    middlewares:
        type: array
        description: middlewares names to use
        defaultDescription: uses grpc-server.service-defaults.middlewares
        items:
            type: string
            description: middleware component name
)");
}

}  // namespace ugrpc::server

USERVER_NAMESPACE_END
