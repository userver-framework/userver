#pragma once

/// @file userver/ugrpc/server/middlewares/congestion_control/component.hpp
/// @brief @copybrief
/// ugrpc::server::middlewares::congestion_control::Component

#include <userver/ugrpc/server/middlewares/base.hpp>

USERVER_NAMESPACE_BEGIN

/// Server congestion_control middleware
namespace ugrpc::server::middlewares::congestion_control {

class Middleware;

/// @ingroup userver_components userver_base_classes
///
/// @brief Component for gRPC server logging
class Component final : public MiddlewareComponentBase {
 public:
  /// @ingroup userver_component_names
  /// @brief The default name of
  /// ugrpc::server::middlewares::congestion_control::Component
  static constexpr std::string_view kName = "grpc-server-congestion-control";

  Component(const components::ComponentConfig& config,
            const components::ComponentContext& context);

  std::shared_ptr<MiddlewareBase> GetMiddleware() override;

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  std::shared_ptr<Middleware> middleware_;
};

}  // namespace ugrpc::server::middlewares::congestion_control

USERVER_NAMESPACE_END
