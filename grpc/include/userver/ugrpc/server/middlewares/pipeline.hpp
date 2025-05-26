#pragma once

/// @file userver/ugrpc/server/middlewares/pipeline.hpp
/// @brief @copybrief ugrpc::server::MiddlewarePipelineComponent

#include <string_view>

#include <userver/middlewares/pipeline.hpp>

#include <userver/ugrpc/server/middlewares/base.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server {

/// @ingroup userver_components userver_grpc_server_middlewares
///
/// @brief Component to create middlewares pipeline.
///
/// You must register your server middleware in this component.
/// Use `MiddlewareDependencyBuilder` to set a dependency of your middleware from others.
///
/// ## Static options:
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// middlewares | middlewares names and configs to use | `{}`
///
/// ## Static config example
///
/// @snippet grpc/functional_tests/middleware_server/static_config.yaml middleware pipeline component config

class MiddlewarePipelineComponent final : public USERVER_NAMESPACE::middlewares::impl::AnyMiddlewarePipelineComponent {
public:
    /// @ingroup userver_component_names
    /// @brief The default name of ugrpc::middlewares::MiddlewarePipelineComponent for the server side.
    static constexpr std::string_view kName = "grpc-server-middlewares-pipeline";

    MiddlewarePipelineComponent(const components::ComponentConfig& config, const components::ComponentContext& context);
};

}  // namespace ugrpc::server

template <>
inline constexpr bool components::kHasValidate<ugrpc::server::MiddlewarePipelineComponent> = true;

template <>
inline constexpr auto components::kConfigFileMode<ugrpc::server::MiddlewarePipelineComponent> =
    ConfigFileMode::kNotRequired;

USERVER_NAMESPACE_END
