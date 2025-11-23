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
/// ## Static options:
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// user-agent | The name of the current deploy unit that will appear in `x-origin` metadata | Do not send `x-origin`
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
