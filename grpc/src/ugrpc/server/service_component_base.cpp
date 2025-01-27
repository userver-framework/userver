#include <userver/ugrpc/server/service_component_base.hpp>

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/utils/assert.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <ugrpc/server/impl/parse_config.hpp>
#include <userver/ugrpc/server/middlewares/base.hpp>
#include <userver/ugrpc/server/middlewares/pipeline.hpp>
#include <userver/ugrpc/server/server_component.hpp>
#include <userver/ugrpc/server/service_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server {

ServiceComponentBase::ServiceComponentBase(
    const components::ComponentConfig& config,
    const components::ComponentContext& context
)
    : ComponentBase(config, context),
      server_(context.FindComponent<ServerComponent>()),
      config_(server_.ParseServiceConfig(config, context)) {
    const auto conf = config.As<impl::MiddlewareServiceConfig>();
    for (const auto& mid :
         context.FindComponent<MiddlewarePipelineComponent>().GetPipeline().GetPerServiceMiddlewares(conf)) {
        config_.middlewares.push_back(context.FindComponent<MiddlewareComponentBase>(mid).GetMiddleware());
    }
}

void ServiceComponentBase::RegisterService(ServiceBase& service) {
    UINVARIANT(!registered_.exchange(true), "Register must only be called once");

    server_.GetServer().AddService(service, std::move(config_));
}

void ServiceComponentBase::RegisterService(GenericServiceBase& service) {
    UINVARIANT(!registered_.exchange(true), "Register must only be called once");
    server_.GetServer().AddService(service, std::move(config_));
}

yaml_config::Schema ServiceComponentBase::GetStaticConfigSchema() {
    return yaml_config::MergeSchemas<components::ComponentBase>(R"(
type: object
description: base class for all the gRPC service components
additionalProperties: false
properties:
    task-processor:
        type: string
        description: the task processor to use for responses
        defaultDescription: uses grpc-server.service-defaults.task-processor
    disable-user-pipeline-middlewares:
        type: boolean
        description: flag to disable groups::User middlewares from pipeline
        defaultDescription: false
    disable-all-pipeline-middlewares:
        type: boolean
        description: flag to disable all middlewares from pipline
        defaultDescription: false
    middlewares:
        type: object
        description: overloads of configs of middlewares per service
        additionalProperties:
            type: object
            description: a middleware config
            additionalProperties: false
            properties:
                enabled:
                    type: boolean
                    description: enable middleware in the list
        properties: {}
)");
}

}  // namespace ugrpc::server

USERVER_NAMESPACE_END
