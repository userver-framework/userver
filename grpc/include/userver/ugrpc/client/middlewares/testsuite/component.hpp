#pragma once

/// @file userver/ugrpc/client/middlewares/testsuite/component.hpp
/// @brief @copybrief ugrpc::client::middlewares::testsuite::Component

#include <userver/ugrpc/client/middlewares/base.hpp>

USERVER_NAMESPACE_BEGIN

/// Client testsuite middleware
/// @see @ref scripts/docs/en/userver/grpc/client_middlewares.md
/// @see @ref ugrpc::client::middlewares::testsuite::Component
namespace ugrpc::client::middlewares::testsuite {

/// @ingroup userver_components
///
/// @brief Component for gRPC client testsuite support
///
/// The component supports testsuite errors thrown from the mockserver, such as `NetworkError`, `TimeoutError`.
///
/// @see @ref pytest_userver.plugins.grpc.mockserver.grpc_mockserver "grpc_mockserver"
/// @see @ref pytest_userver.grpc._mocked_errors.TimeoutError "pytest_userver.grpc.TimeoutError"
/// @see @ref pytest_userver.grpc._mocked_errors.NetworkError "pytest_userver.grpc.NetworkError"
///
/// The component does **not** have any options for service config.
///
/// @see @ref scripts/docs/en/userver/grpc/client_middlewares.md

class Component : public MiddlewareFactoryComponentBase {
public:
    /// @ingroup userver_component_names
    /// @brief The default name of @ref ugrpc::client::middlewares::testsuite::Component.
    static constexpr std::string_view kName = "grpc-client-middleware-testsuite";

    Component(const components::ComponentConfig& config, const components::ComponentContext& context);

    std::shared_ptr<const MiddlewareBase>
    CreateMiddleware(const ClientInfo& info, const yaml_config::YamlConfig& middleware_config) const override;
};

}  // namespace ugrpc::client::middlewares::testsuite

template <>
inline constexpr auto components::kConfigFileMode<ugrpc::client::middlewares::testsuite::Component> =
    ConfigFileMode::kNotRequired;

USERVER_NAMESPACE_END
