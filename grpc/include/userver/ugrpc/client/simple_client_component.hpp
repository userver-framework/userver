#pragma once

/// @file userver/ugrpc/client/simple_client_component.hpp
/// @brief @copybrief ugrpc::client::SimpleClientComponent

#include <memory>

#include <userver/components/component_base.hpp>
#include <userver/utils/not_null.hpp>

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
    static ClientFactory& FindFactory(
        const components::ComponentConfig& config,
        const components::ComponentContext& context
    );

    static ClientSettings MakeClientSettings(
        const components::ComponentConfig& config,
        const dynamic_config::Key<ClientQos>* client_qos
    );
};

}  // namespace impl

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
///
/// ## Static options of ugrpc::client::CommonComponent :
/// @include{doc} scripts/docs/en/components_schema/grpc/src/ugrpc/client/simple_client_component.md
///
/// Options inherited from @ref components::ComponentBase :
/// @include{doc} scripts/docs/en/components_schema/core/src/components/impl/component_base.md
template <typename Client>
class SimpleClientComponent : public impl::SimpleClientComponentAny {
public:
    /// Main component's constructor.
    SimpleClientComponent(const components::ComponentConfig& config, const components::ComponentContext& context)
        : SimpleClientComponentAny(config, context),
          client_(utils::MakeSharedRef<
                  Client>(FindFactory(config, context).MakeClient<Client>(MakeClientSettings(config, nullptr))))
    {}

    /// To use a ClientQos config, derive from SimpleClientComponent and provide the parameter at construction:
    /// @snippet samples/grpc_service/src/greeter_client.hpp  component
    SimpleClientComponent(
        const components::ComponentConfig& config,
        const components::ComponentContext& context,
        const dynamic_config::Key<ClientQos>& client_qos
    )
        : SimpleClientComponentAny(config, context),
          client_(utils::MakeSharedRef<
                  Client>(FindFactory(config, context).MakeClient<Client>(MakeClientSettings(config, &client_qos))))
    {}

    /// @brief Get gRPC service client.
    Client& GetClient() noexcept { return *client_; }

protected:
    /// @brief Get gRPC service client ptr.
    utils::SharedRef<Client> GetClientPtr() noexcept { return client_; }

private:
    utils::SharedRef<Client> client_;
};

}  // namespace ugrpc::client

namespace components {

template <typename Client>
inline constexpr bool kHasValidate<ugrpc::client::SimpleClientComponent<Client>> = true;

}  // namespace components

USERVER_NAMESPACE_END
