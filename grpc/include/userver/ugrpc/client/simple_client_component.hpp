#pragma once

/// @file userver/ugrpc/client/simple_client_component.hpp
/// @brief @copybrief ugrpc::client::SimpleClientComponent

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>

#include <userver/ugrpc/client/client_factory_component.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

class SimpleClientComponentAny : public components::LoggableComponentBase {
 public:
  using components::LoggableComponentBase::LoggableComponentBase;

  static yaml_config::Schema GetStaticConfigSchema();
};

/// @ingroup userver_components
///
/// @brief Template class for a simple gRPC client
///
/// The component is used as a storage of a gRPC client if you're OK with
/// generated client and don't need to wrap it. The client can be fetched using
/// `GetClient` method.
template <typename Client>
class SimpleClientComponent final : public SimpleClientComponentAny {
 public:
  SimpleClientComponent(const components::ComponentConfig& config,
                        const components::ComponentContext& context)
      : SimpleClientComponentAny(config, context),
        client_(context
                    .FindComponent<ClientFactoryComponent>(
                        config["factory-component"].As<std::string>(
                            ClientFactoryComponent::kName))
                    .GetFactory()
                    .MakeClient<Client>(config["endpoint"].As<std::string>())) {
  }

  /// @@brief Get gRPC service client
  Client& GetClient() { return client_; }

 private:
  Client client_;
};

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
