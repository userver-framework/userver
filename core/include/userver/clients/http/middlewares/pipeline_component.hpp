#pragma once

/// @file userver/clients/http/middlewares/pipeline_component.hpp
/// @brief @copybrief clients::http::MiddlewarePipelineComponent

#include <userver/components/component_base.hpp>
#include <userver/middlewares/impl/middleware_pipeline_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http {

/// @ingroup userver_components
///
/// @brief Component that provides list of common middlewares names for all @ref components::HttpClient instances
///
/// ## Static options of @ref clients::http::MiddlewarePipelineComponent :
/// @include{doc} scripts/docs/en/components_schema/core/src/clients/http/middlewares/pipeline_component.md
///
/// ## Static configuration example:
/// @snippet components/common_component_list_test.cpp  Sample http client component config
class MiddlewarePipelineComponent final : public components::ComponentBase {
public:
    /// @ingroup userver_component_names
    /// @brief The default name of clients::http::MiddlewarePipelineComponent component
    static constexpr std::string_view kName = "http-client-middleware-pipeline";

    MiddlewarePipelineComponent(const components::ComponentConfig&, const components::ComponentContext&);

    const USERVER_NAMESPACE::middlewares::impl::MiddlewaresConfig& GetMiddlewaresConfig() const;

    static yaml_config::Schema GetStaticConfigSchema();

private:
    USERVER_NAMESPACE::middlewares::impl::MiddlewaresConfig config_;
};

}  // namespace clients::http

template <>
inline constexpr bool components::kHasValidate<clients::http::MiddlewarePipelineComponent> = true;

template <>
inline constexpr auto
    components::kConfigFileMode<clients::http::MiddlewarePipelineComponent> = ConfigFileMode::kNotRequired;

USERVER_NAMESPACE_END
