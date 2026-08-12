#pragma once

/// @file userver/ugrpc/server/middlewares/deadline_propagation/component.hpp
/// @brief @copybrief ugrpc::server::middlewares::deadline_propagation::Component

#include <userver/ugrpc/server/middlewares/deadline_propagation/middleware.hpp>

USERVER_NAMESPACE_BEGIN

/// Server deadline propagation middleware
/// @see @ref scripts/docs/en/userver/grpc/server_middlewares.md
/// @see @ref ugrpc::server::middlewares::deadline_propagation::Component
namespace ugrpc::server::middlewares::deadline_propagation {

// clang-format off

/// @ingroup userver_components
///
/// @brief Component for gRPC server deadline propagation
/// @see @ref scripts/docs/en/userver/deadline_propagation.md
///
/// ## Static options of ugrpc::server::middlewares::deadline_propagation::Component :
/// @include{doc} scripts/docs/en/components_schema/grpc/src/ugrpc/server/middlewares/deadline_propagation/component.md
///
/// ## Static configuration example:
///
/// @snippet grpc/functional_tests/basic_chaos/static_config.yaml Sample grpc server deadline propagation middleware component config
///
/// @see @ref scripts/docs/en/userver/grpc/server_middlewares.md

// clang-format on

class Component final : public MiddlewareFactoryComponentBase {
public:
    /// @ingroup userver_component_names
    /// @brief The default name of @ref ugrpc::server::middlewares::deadline_propagation::Component.
    static constexpr std::string_view kName = "grpc-server-deadline-propagation";

    Component(const components::ComponentConfig& config, const components::ComponentContext& context);

    static yaml_config::Schema GetStaticConfigSchema();

    yaml_config::Schema GetMiddlewareConfigSchema() const override;

    std::shared_ptr<const MiddlewareBase> CreateMiddleware(
        const ugrpc::server::ServiceInfo&,
        const yaml_config::YamlConfig& middleware_config
    ) const override;
};

}  // namespace ugrpc::server::middlewares::deadline_propagation

template <>
inline constexpr bool components::kHasValidate<ugrpc::server::middlewares::deadline_propagation::Component> = true;

template <>
inline constexpr auto components::kConfigFileMode<
    ugrpc::server::middlewares::deadline_propagation::Component> = ConfigFileMode::kNotRequired;

USERVER_NAMESPACE_END
