#include <userver/ugrpc/server/service_component_base.hpp>

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/formats/common/merge.hpp>
#include <userver/formats/yaml/value_builder.hpp>
#include <userver/middlewares/runner.hpp>
#include <userver/utils/assert.hpp>
#include <userver/yaml_config/merge_schemas.hpp>
#include <userver/yaml_config/yaml_config.hpp>

#include <ugrpc/server/impl/parse_config.hpp>
#include <userver/ugrpc/server/middlewares/pipeline.hpp>
#include <userver/ugrpc/server/server_component.hpp>
#include <userver/ugrpc/server/service_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server {

ServiceComponentBase::ServiceComponentBase(
    const components::ComponentConfig& config,
    const components::ComponentContext& context
)
    : impl::MiddlewareRunnerComponentBase(config, context, MiddlewarePipelineComponent::kName),
      server_(context.FindComponent<ServerComponent>()),
      config_(server_.ParseServiceConfig(config, context)),
      info_(ServiceInfo{config.Name()}) {
    config_.middlewares = CreateMiddlewares(*info_);
}

ServiceComponentBase::~ServiceComponentBase() = default;

void ServiceComponentBase::RegisterService(ServiceBase& service) {
    UINVARIANT(!registered_.exchange(true), "Register must only be called once");

    server_.GetServer().AddService(service, std::move(config_));
}

void ServiceComponentBase::RegisterService(GenericServiceBase& service) {
    UINVARIANT(!registered_.exchange(true), "Register must only be called once");
    server_.GetServer().AddService(service, std::move(config_));
}

yaml_config::Schema ServiceComponentBase::GetStaticConfigSchema() {
    return yaml_config::MergeSchemas<impl::MiddlewareRunnerComponentBase>(R"(
type: object
description: base class for all the gRPC service components
additionalProperties: false
properties:
    task-processor:
        type: string
        description: the task processor to use for responses
        defaultDescription: uses grpc-server.service-defaults.task-processor
    status-codes-log-level:
        type: object
        properties: {}
        additionalProperties:
            type: string
            description: log level
        description: |
            gRPC status code string -> log level map.
            Allows overriding the log level for the span for individual statuses.
            See grpcpp StatusCode documentation for the list of possible values:
            https://grpc.github.io/grpc/cpp/namespacegrpc.html#aff1730578c90160528f6a8d67ef5c43b

            Example:
              -  FAILED_PRECONDITION: info
)");
}

}  // namespace ugrpc::server

USERVER_NAMESPACE_END
