#pragma once

/// @file userver/ugrpc/client/simple_client_component.hpp
/// @brief @copybrief ugrpc::client::SimpleClientComponent

#include <userver/components/component_base.hpp>

#include <userver/ugrpc/client/client_factory.hpp>
#include <userver/ugrpc/client/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

namespace impl {

class SimpleClientComponentAny : public components::ComponentBase {
public:
    using components::ComponentBase::ComponentBase;

    static yaml_config::Schema GetStaticConfigSchema();

protected:
    static ClientFactory&
    FindFactory(const components::ComponentConfig& config, const components::ComponentContext& context);

    static ClientSettings
    MakeClientSettings(const components::ComponentConfig& config, const dynamic_config::Key<ClientQos>* client_qos);
};

}  // namespace impl

// clang-format off

/// @ingroup userver_components
///
/// @brief Template class for a simple gRPC client
///
/// The component is used as a storage of a gRPC client if you're OK with
/// generated client and don't need to wrap it. The client can be fetched using
/// `GetClient` method.
///
/// Example usage:
///
/// ```cpp
/// int main(...)
/// {
///    ...
///    component_list.Append<ugrpc::client::SimpleClientComponent<HelloClient>>("hello-client");
///    ...
/// }
///
/// MyComponent::MyComponent(const components::ComponentConfig& config,
///                          const components::ComponentContext& context)
/// {
///    HelloClient& client = context.FindComponent<
///        ugrpc::client::SimpleClientComponent<HelloClient>>("hello-client").GetClient();
///    ... use client ...
/// }
/// ```
///
/// ## Static config options:
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// endpoint | URL of the gRPC service | --
/// client-name | name of the gRPC server we talk to, for diagnostics | <uses the component name>
/// dedicated-channel-counts | a map of a full rpc name (`full.service.Name/MethodName`) in channel count. Used for high-load methods | -
/// factory-component | ClientFactoryComponent name to use for client creation | --

// clang-format on

template <typename Client>
class SimpleClientComponent : public impl::SimpleClientComponentAny {
public:
    /// Main component's constructor.
    SimpleClientComponent(const components::ComponentConfig& config, const components::ComponentContext& context)
        : SimpleClientComponentAny(config, context),
          client_(FindFactory(config, context).MakeClient<Client>(MakeClientSettings(config, nullptr))) {}

    /// To use a ClientQos config,
    SimpleClientComponent(
        const components::ComponentConfig& config,
        const components::ComponentContext& context,
        const dynamic_config::Key<ClientQos>& client_qos
    )
        : SimpleClientComponentAny(config, context),
          client_(FindFactory(config, context).MakeClient<Client>(MakeClientSettings(config, &client_qos))) {}

    /// @@brief Get gRPC service client
    Client& GetClient() { return client_; }

private:
    Client client_;
};

}  // namespace ugrpc::client

namespace components {

template <typename Client>
inline constexpr bool kHasValidate<ugrpc::client::SimpleClientComponent<Client>> = true;

}  // namespace components

USERVER_NAMESPACE_END
