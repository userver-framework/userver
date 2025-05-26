#pragma once

/// @file userver/ugrpc/client/middlewares/pipeline.hpp
/// @brief @copybrief ugrpc::client::MiddlewarePipelineComponent

#include <string_view>

#include <userver/middlewares/impl/pipeline_creator_interface.hpp>
#include <userver/middlewares/pipeline.hpp>

#include <userver/ugrpc/client/middlewares/base.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

/// @ingroup userver_components userver_grpc_client_middlewares
///
/// @brief Component to create middlewares pipeline.
///
/// You must register your client middleware in this component.
/// Use middlewares::MiddlewareDependencyBuilder to set a dependency of your middleware from others.
///
/// ## Static options:
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// middlewares | middlewares names and configs to use | `{}`
///
/// ## Static config example
///
/// @snippet samples/grpc_middleware_service/configs/static_config.yaml static config grpc-auth-client

class MiddlewarePipelineComponent final : public USERVER_NAMESPACE::middlewares::impl::AnyMiddlewarePipelineComponent {
public:
    /// @ingroup userver_component_names
    /// @brief The default name of ugrpc::middlewares::MiddlewarePipelineComponent for the client side.
    static constexpr std::string_view kName = "grpc-client-middlewares-pipeline";

    MiddlewarePipelineComponent(const components::ComponentConfig& config, const components::ComponentContext& context);
};

namespace impl {

/// @ingroup userver_grpc_client_middlewares
///
/// @brief specialization of PipelineCreatorInterface interface to create client middlewares.
using MiddlewarePipelineCreator =
    USERVER_NAMESPACE::middlewares::impl::PipelineCreatorInterface<MiddlewareBase, ClientInfo>;

}  // namespace impl

}  // namespace ugrpc::client

template <>
inline constexpr bool components::kHasValidate<ugrpc::client::MiddlewarePipelineComponent> = true;

template <>
inline constexpr auto components::kConfigFileMode<ugrpc::client::MiddlewarePipelineComponent> =
    ConfigFileMode::kNotRequired;

USERVER_NAMESPACE_END
