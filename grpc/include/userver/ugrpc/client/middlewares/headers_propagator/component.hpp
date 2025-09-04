#pragma once

/// @file userver/ugrpc/client/middlewares/headers_propagator/component.hpp
/// @brief @copybrief ugrpc::client::middlewares::headers_propagator::Component

#include <userver/ugrpc/client/middlewares/base.hpp>

USERVER_NAMESPACE_BEGIN

/// Server headers_propagator middleware
/// @see @ref scripts/docs/en/userver/grpc/client_middlewares.md
/// @see @ref ugrpc::client::middlewares::headers_propagator::Component
namespace ugrpc::client::middlewares::headers_propagator {

/// @ingroup userver_components userver_base_classes
///
/// @brief gRPC client middleware for sending headers stored by the respective HTTP and gRPC server middlewares.
///
/// ## Static options:
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// skip-headers | map from metadata fields (headers) to whether it should be skipped | {}
///
/// @see @ref scripts/docs/en/userver/grpc/client_middlewares.md

class Component final : public MiddlewareFactoryComponentBase {
public:
    /// @ingroup userver_component_names
    /// @brief The default name of ugrpc::client::middlewares::headers_propagator::Component.
    static constexpr std::string_view kName = "grpc-client-headers-propagator";

    Component(const components::ComponentConfig& config, const components::ComponentContext& context);

    ~Component() override;

    static yaml_config::Schema GetStaticConfigSchema();

    yaml_config::Schema GetMiddlewareConfigSchema() const override;

    std::shared_ptr<const MiddlewareBase>
    CreateMiddleware(const ugrpc::client::ClientInfo&, const yaml_config::YamlConfig& middleware_config) const override;
};

}  // namespace ugrpc::client::middlewares::headers_propagator

template <>
inline constexpr auto components::kConfigFileMode<ugrpc::client::middlewares::headers_propagator::Component> =
    ConfigFileMode::kNotRequired;

USERVER_NAMESPACE_END
