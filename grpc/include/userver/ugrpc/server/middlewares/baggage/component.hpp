#pragma once

/// @file userver/ugrpc/server/middlewares/baggage/component.hpp
/// @brief @copybrief
/// ugrpc::server::middlewares::baggage::Component

#include <userver/ugrpc/server/middlewares/base.hpp>

USERVER_NAMESPACE_BEGIN

/// Server baggae middleware
namespace ugrpc::server::middlewares::baggage {

/// @ingroup userver_components userver_base_classes
///
/// @brief Component for gRPC server baggage
class Component final : public MiddlewareComponentBase {
 public:
  /// @ingroup userver_component_names
  /// @brief The default name of ugrpc::server::middlewares::baggage::Component
  static constexpr std::string_view kName = "grpc-server-baggage";

  Component(const components::ComponentConfig& config,
            const components::ComponentContext& context);

  std::shared_ptr<MiddlewareBase> GetMiddleware() override;

  static yaml_config::Schema GetStaticConfigSchema();
};

}  // namespace ugrpc::server::middlewares::baggage

USERVER_NAMESPACE_END
