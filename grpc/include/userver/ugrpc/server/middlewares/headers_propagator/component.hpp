#pragma once

/// @file userver/ugrpc/server/middlewares/headers_propagator/component.hpp
/// @brief @copybrief
/// ugrpc::server::middlewares::headers_propagator::Component

#include <userver/ugrpc/server/middlewares/base.hpp>

USERVER_NAMESPACE_BEGIN

/// Server baggae middleware
namespace ugrpc::server::middlewares::headers_propagator {

/// @ingroup userver_components userver_base_classes
///
/// @brief Component for gRPC server headers_propagator
class Component final : public MiddlewareComponentBase {
 public:
  /// @ingroup userver_component_names
  /// @brief The default name of
  /// ugrpc::server::middlewares::headers_propagator::Component
  static constexpr std::string_view kName = "grpc-server-headers-propagator";

  Component(const components::ComponentConfig& config,
            const components::ComponentContext& context);

  std::shared_ptr<MiddlewareBase> GetMiddleware() override;

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  const components::ComponentConfig& config_;
};

}  // namespace ugrpc::server::middlewares::headers_propagator

USERVER_NAMESPACE_END
