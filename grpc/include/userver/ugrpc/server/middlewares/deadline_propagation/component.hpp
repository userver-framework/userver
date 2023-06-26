#pragma once

/// @file userver/ugrpc/server/middlewares/deadline_propagation/component.hpp
/// @brief @copybrief
/// ugrpc::server::middlewares::deadline_propagation::Component

#include <userver/ugrpc/server/middlewares/base.hpp>

USERVER_NAMESPACE_BEGIN

/// Server logging middleware
namespace ugrpc::server::middlewares::deadline_propagation {

/// @ingroup userver_components userver_base_classes
///
/// @brief Component for gRPC server logging

class Component final : public MiddlewareComponentBase {
 public:
  static constexpr std::string_view kName = "grpc-server-deadline-propagation";

  Component(const components::ComponentConfig& config,
            const components::ComponentContext& context);

  std::shared_ptr<MiddlewareBase> GetMiddleware() override;

  static yaml_config::Schema GetStaticConfigSchema();
};

}  // namespace ugrpc::server::middlewares::deadline_propagation

USERVER_NAMESPACE_END
