#pragma once

/// @file userver/ugrpc/server/middlewares/deadline_propagation/component.hpp
/// @brief @copybrief
/// ugrpc::server::middlewares::deadline_propagation::Component

#include <userver/ugrpc/server/middlewares/base.hpp>

USERVER_NAMESPACE_BEGIN

/// Server deadline propagation middleware
namespace ugrpc::server::middlewares::deadline_propagation {

// clang-format off

/// @ingroup userver_components userver_base_classes
///
/// @brief Component for gRPC server deadline propagation
/// @see @ref scripts/docs/en/userver/deadline_propagation.md
///
/// The component does **not** have any options for service config.
///
/// ## Static configuration example:
///
/// @snippet grpc/functional_tests/basic_chaos/static_config.yaml Sample grpc server deadline propagation middleware component config

// clang-format on

class Component final : public MiddlewareComponentBase {
 public:
  /// @ingroup userver_component_names
  /// @brief The default name of
  // ugrpc::server::middlewares::deadline_propagation::Component
  static constexpr std::string_view kName = "grpc-server-deadline-propagation";

  Component(const components::ComponentConfig& config,
            const components::ComponentContext& context);

  std::shared_ptr<MiddlewareBase> GetMiddleware() override;

  static yaml_config::Schema GetStaticConfigSchema();
};

}  // namespace ugrpc::server::middlewares::deadline_propagation

USERVER_NAMESPACE_END
