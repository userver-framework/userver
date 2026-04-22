#pragma once

/// @file userver/ugrpc/server/middlewares/graceful_shutdown_headers/component.hpp
/// @brief @copybrief ugrpc::server::middlewares::graceful_shutdown_headers::Component

#include <userver/components/state.hpp>
#include <userver/dynamic_config/source.hpp>
#include <userver/ugrpc/server/middlewares/base.hpp>

USERVER_NAMESPACE_BEGIN

/// Middleware populating graceful shutdown headers
/// @see @ref scripts/docs/en/userver/grpc/server_middlewares.md
/// @see @ref ugrpc::server::middlewares::graceful_shutdown_headers::Component
namespace ugrpc::server::middlewares::graceful_shutdown_headers {

/// @ingroup userver_components userver_base_classes
///
/// @brief Component for gRPC server graceful_shutdown_headers
///
/// ## Static options inherited from @ref middlewares::MiddlewareFactoryComponentBase :
/// @include{doc} scripts/docs/en/components_schema/core/src/middlewares/factory_component_base.md
///
/// Options inherited from @ref components::ComponentBase :
/// @include{doc} scripts/docs/en/components_schema/core/src/components/impl/component_base.md
///
/// @see @ref scripts/docs/en/userver/grpc/server_middlewares.md
class Component final : public MiddlewareFactoryComponentBase {
public:
    /// @ingroup userver_component_names
    /// @brief The default name of ugrpc::server::middlewares::headers_propagator::Component
    static constexpr std::string_view kName = "grpc-server-graceful-shutdown-headers";

    Component(const components::ComponentConfig& config, const components::ComponentContext& context);

    std::shared_ptr<const MiddlewareBase>
    CreateMiddleware(const ugrpc::server::ServiceInfo&, const yaml_config::YamlConfig&) const override;

private:
    const components::State state_;
    dynamic_config::Source source_;
};

}  // namespace ugrpc::server::middlewares::graceful_shutdown_headers

template <>
inline constexpr bool components::kHasValidate<ugrpc::server::middlewares::graceful_shutdown_headers::Component> = true;

template <>
inline constexpr auto components::kConfigFileMode<
    ugrpc::server::middlewares::graceful_shutdown_headers::Component> = ConfigFileMode::kNotRequired;

USERVER_NAMESPACE_END
