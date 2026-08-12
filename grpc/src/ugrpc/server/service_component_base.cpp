#include <userver/ugrpc/server/service_component_base.hpp>

#include <google/protobuf/descriptor.h>

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

#ifndef ARCADIA_ROOT
#include "generated/src/ugrpc/server/service_component_base.yaml.hpp"  // Y_IGNORE
#endif

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server {

ServiceComponentBase::ServiceComponentBase(
    const components::ComponentConfig& config,
    const components::ComponentContext& context
)
    : impl::MiddlewareRunnerComponentBase(config, context, MiddlewarePipelineComponent::kName),
      server_(context.FindComponent<ServerComponent>()),
      config_(server_.ParseServiceConfig(config, context)),
      component_name_(context.GetComponentName())
{}

ServiceComponentBase::~ServiceComponentBase() = default;

void ServiceComponentBase::RegisterService(ServiceBase& service) {
    UINVARIANT(!registered_.exchange(true), "Register must only be called once");

    ServiceInfo service_info;
    service_info.service_component_name = component_name_;
    service_info.full_service_name = service.GetMetadata(utils::impl::InternalTag{}).service_full_name;
    service_info.service_descriptor =
        google::protobuf::DescriptorPool::generated_pool()->FindServiceByName(*service_info.full_service_name);
    config_.middlewares = CreateMiddlewares(service_info);

    server_.GetServer().AddService(service, std::move(config_));
}

void ServiceComponentBase::RegisterService(GenericServiceBase& service) {
    UINVARIANT(!registered_.exchange(true), "Register must only be called once");

    ServiceInfo service_info;
    service_info.service_component_name = component_name_;
    config_.middlewares = CreateMiddlewares(service_info);

    server_.GetServer().AddService(service, std::move(config_));
}

yaml_config::Schema ServiceComponentBase::GetStaticConfigSchema() {
    return yaml_config::MergeSchemasFromResource<
        impl::MiddlewareRunnerComponentBase>("src/ugrpc/server/service_component_base.yaml");
}

}  // namespace ugrpc::server

USERVER_NAMESPACE_END
