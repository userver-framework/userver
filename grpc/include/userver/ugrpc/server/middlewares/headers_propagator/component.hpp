#pragma once

/// @file userver/ugrpc/server/middlewares/headers_propagator/component.hpp
/// @brief @copybrief ugrpc::server::middlewares::headers_propagator::Component

#include <userver/ugrpc/server/middlewares/base.hpp>

USERVER_NAMESPACE_BEGIN

/// Server headers_propagator middleware
namespace ugrpc::server::middlewares::headers_propagator {

// clang-format off

/// @ingroup userver_components userver_base_classes
///
/// @brief Component for gRPC server headers_propagator
///
/// ## Static options:
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// headers | array of metadata fields (headers) to propagate | empty

// clang-format on

class Component final : public MiddlewareFactoryComponentBase {
public:
    /// @ingroup userver_component_names
    /// @brief The default name of
    /// ugrpc::server::middlewares::headers_propagator::Component
    static constexpr std::string_view kName = "grpc-server-headers-propagator";

    Component(const components::ComponentConfig& config, const components::ComponentContext& context);

    static yaml_config::Schema GetStaticConfigSchema();

    yaml_config::Schema GetMiddlewareConfigSchema() const override;

    std::shared_ptr<const MiddlewareBase> CreateMiddleware(
        const ugrpc::server::ServiceInfo&,
        const yaml_config::YamlConfig& middleware_config
    ) const override;
};

}  // namespace ugrpc::server::middlewares::headers_propagator

template <>
inline constexpr bool components::kHasValidate<ugrpc::server::middlewares::headers_propagator::Component> = true;

template <>
inline constexpr auto components::kConfigFileMode<ugrpc::server::middlewares::headers_propagator::Component> =
    ConfigFileMode::kNotRequired;

USERVER_NAMESPACE_END
