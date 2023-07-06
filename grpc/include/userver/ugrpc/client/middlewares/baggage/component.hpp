#pragma once

/// @file userver/ugrpc/client/baggage/component.hpp
/// @brief @copybrief ugrpc::client::middlewares::baggage::Component

#include <userver/ugrpc/client/middlewares/base.hpp>

USERVER_NAMESPACE_BEGIN

/// Client baggage middleware
namespace ugrpc::client::middlewares::baggage {

/// @ingroup userver_components
///
/// @brief Component for gRPC client baggage

class Component final : public MiddlewareComponentBase {
 public:
  static constexpr std::string_view kName = "grpc-client-baggage";

  Component(const components::ComponentConfig& config,
            const components::ComponentContext& context);

  std::shared_ptr<const MiddlewareFactoryBase> GetMiddlewareFactory() override;

  static yaml_config::Schema GetStaticConfigSchema();
};

}  // namespace ugrpc::client::middlewares::baggage

USERVER_NAMESPACE_END
