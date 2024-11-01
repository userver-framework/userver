#include <userver/ugrpc/client/simple_client_component.hpp>

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <userver/ugrpc/client/client_factory_component.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

yaml_config::Schema SimpleClientComponentAny::GetStaticConfigSchema() {
    return yaml_config::MergeSchemas<components::ComponentBase>(R"(
type: object
description: Provides a ClientFactory in the component system
additionalProperties: false
properties:
    endpoint:
        type: string
        description: URL of the gRPC service
    client-name:
        type: string
        description: name of the gRPC server we talk to, for diagnostics
    factory-component:
        type: string
        description: ClientFactoryComponent name to use for client creation
    dedicated-channel-counts:
        type: object
        description: a map of a rpc method in the channel count. Used for high-load methods
        defaultDescription: '{}'
        additionalProperties:
            type: integer
            description: a full path to service method, must be a string or integer
            minimum: 1
        properties: {}
)");
}

ClientFactory& SimpleClientComponentAny::FindFactory(
    const components::ComponentConfig& config,
    const components::ComponentContext& context
) {
    return context
        .FindComponent<ClientFactoryComponent>(config["factory-component"].As<std::string>(ClientFactoryComponent::kName
        ))
        .GetFactory();
}

ClientSettings SimpleClientComponentAny::MakeClientSettings(
    const components::ComponentConfig& config,
    const dynamic_config::Key<ClientQos>* client_qos
) {
    ClientSettings client_settings;
    client_settings.client_name = config["client-name"].As<std::string>(config.Name());
    client_settings.endpoint = config["endpoint"].As<std::string>();
    client_settings.client_qos = client_qos;
    client_settings.dedicated_methods_config =
        config["dedicated-channel-counts"].As<DedicatedMethodsConfig>(client_settings.dedicated_methods_config);
    return client_settings;
}

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
