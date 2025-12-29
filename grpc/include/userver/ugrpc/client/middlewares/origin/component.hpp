#pragma once

/// @file userver/ugrpc/client/middlewares/origin/component.hpp
/// @brief @copybrief ugrpc::client::middlewares::origin::Component

#include <userver/ugrpc/client/middlewares/base.hpp>

USERVER_NAMESPACE_BEGIN

/// Server origin middleware
/// @see @ref scripts/docs/en/userver/grpc/client_middlewares.md
/// @see @ref ugrpc::client::middlewares::origin::Component
namespace ugrpc::client::middlewares::origin {

/// @ingroup userver_components
///
/// @brief gRPC client middleware that sets `x-origin` metadata. gRPC server will then copy that to `useragent` tag
/// in server request log.
///
/// ## Static options of ugrpc::client::middlewares::log::Component:
/// @include{doc} scripts/docs/en/components_schema/grpc/src/ugrpc/client/middlewares/origin/component.md
///
/// Options inherited from @ref middlewares::MiddlewareFactoryComponentBase :
/// @include{doc} scripts/docs/en/components_schema/core/src/middlewares/factory_component_base.md
///
/// Options inherited from @ref components::ComponentBase :
/// @include{doc} scripts/docs/en/components_schema/core/src/components/impl/component_base.md
///
/// Config example:
/// @snippet samples/grpc_middleware_service/configs/static_config.yaml grpc-client-origin
///
/// @see @ref scripts/docs/en/userver/grpc/client_middlewares.md
class Component final : public MiddlewareFactoryComponentBase {
public:
    /// @ingroup userver_component_names
    /// @brief The default name of ugrpc::client::middlewares::headers_propagator::Component.
    static constexpr std::string_view kName = "grpc-client-origin";

    Component(const components::ComponentConfig& config, const components::ComponentContext& context);

    ~Component() override;

    static yaml_config::Schema GetStaticConfigSchema();

    yaml_config::Schema GetMiddlewareConfigSchema() const override;

    std::shared_ptr<const MiddlewareBase> CreateMiddleware(
        const ugrpc::client::ClientInfo&,
        const yaml_config::YamlConfig& middleware_config
    ) const override;
};

}  // namespace ugrpc::client::middlewares::origin

USERVER_NAMESPACE_END
