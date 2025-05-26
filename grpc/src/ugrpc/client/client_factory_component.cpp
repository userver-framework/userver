#include <userver/ugrpc/client/client_factory_component.hpp>

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/dynamic_config/storage/component.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <ugrpc/client/impl/client_factory_config.hpp>
#include <userver/ugrpc/client/common_component.hpp>
#include <userver/ugrpc/client/middlewares/pipeline.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

namespace {

const storages::secdist::SecdistConfig* GetSecdist(const components::ComponentContext& context) {
    if (const auto* const component = context.FindComponentOptional<components::Secdist>()) {
        return &component->Get();
    }
    return nullptr;
}

}  // namespace

ClientFactoryComponent::ClientFactoryComponent(
    const components::ComponentConfig& config,
    const components::ComponentContext& context
)
    : impl::MiddlewareRunnerComponentBase(config, context, MiddlewarePipelineComponent::kName) {
    auto& client_common_component = context.FindComponent<CommonComponent>();

    const auto config_source = context.FindComponent<components::DynamicConfig>().GetSource();

    auto& testsuite_grpc = context.FindComponent<components::TestsuiteSupport>().GetGrpcControl();

    auto factory_config = config.As<impl::ClientFactoryConfig>();

    const auto* secdist = GetSecdist(context);

    factory_.emplace(
        MakeFactorySettings(std::move(factory_config), secdist),
        client_common_component.blocking_task_processor_,
        *this,  // impl::PipelineCreatorInterface&
        client_common_component.completion_queues_,
        client_common_component.client_statistics_storage_,
        testsuite_grpc,  //
        config_source
    );
}

ClientFactory& ClientFactoryComponent::GetFactory() { return *factory_; }

yaml_config::Schema ClientFactoryComponent::GetStaticConfigSchema() {
    return yaml_config::MergeSchemas<impl::MiddlewareRunnerComponentBase>(R"(
type: object
description: Provides a ClientFactory in the component system
additionalProperties: false
properties:
    channel-args:
        type: object
        description: a map of channel arguments, see gRPC Core docs
        defaultDescription: '{}'
        additionalProperties:
            type: string
            description: value of channel argument, must be string or integer
        properties: {}
    auth-type:
        type: string
        description: an optional authentication method
        defaultDescription: insecure
        enum:
          - insecure
          - ssl
    auth-token:
        type: string
        description: auth token name from secdist
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
)");
}

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
