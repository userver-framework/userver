#pragma once

/// @file userver/ugrpc/client/middlewares/deadline_propagation/component.hpp
/// @brief @copybrief
/// ugrpc::client::middlewares::deadline_propagation::Component

#include <userver/ugrpc/client/middlewares/base.hpp>

USERVER_NAMESPACE_BEGIN

/// Client logging middleware
namespace ugrpc::client::middlewares::deadline_propagation {

// clang-format off

/// @ingroup userver_components
///
/// @brief Component for gRPC client deadline_propagation. Update deadline
/// from TaskInheritedData if it exists and more strict than
/// context deadline.
/// @see @ref scripts/docs/en/userver/deadline_propagation.md
///
/// The component does **not** have any options for service config.
///
/// ## Static configuration example:
///
/// @snippet grpc/functional_tests/basic_chaos/static_config.yaml Sample grpc client deadline propagation middleware component config

// clang-format on

class Component final : public MiddlewareComponentBase {
public:
    /// @ingroup userver_component_names
    /// @brief The default name of
    /// ugrpc::client::middlewares::deadline_propagation::Component
    static constexpr std::string_view kName = "grpc-client-deadline-propagation";

    Component(const components::ComponentConfig& config, const components::ComponentContext& context);

    std::shared_ptr<const MiddlewareFactoryBase> GetMiddlewareFactory() override;

    static yaml_config::Schema GetStaticConfigSchema();
};

}  // namespace ugrpc::client::middlewares::deadline_propagation

USERVER_NAMESPACE_END
