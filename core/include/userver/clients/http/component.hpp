#pragma once

#ifdef USERVER_TVM2_HTTP_CLIENT
#error Use components::Http from clients/http.hpp instead
#endif

/// @file userver/clients/http/component.hpp
/// @brief @copybrief components::HttpClient

#include <userver/clients/http/client.hpp>
#include <userver/clients/http/client_with_middlewares.hpp>
#include <userver/clients/http/component_core.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

/// @ingroup userver_components
///
/// @brief Component that manages @ref clients::http::ClientWithMiddlewares.
///
/// Reuses @ref clients::http::ClientCore from @ref components::HttpClientCore and applies
/// sequence of @ref clients::http::MiddlewareBase to the request.
///
/// Returned references to @ref clients::http::Client live for a lifetime of the
/// component and are safe for concurrent use.
///
/// ## Static options of components::HttpClient :
/// @include{doc} scripts/docs/en/components_schema/core/src/clients/http/component.md
///
/// Options inherited from @ref components::ComponentBase :
/// @include{doc} scripts/docs/en/components_schema/core/src/components/impl/component_base.md
///
/// ## Static configuration example:
///
/// @snippet components/common_component_list_test.cpp  Sample http client component config
class HttpClient final : public ComponentBase {
public:
    /// @ingroup userver_component_names
    /// @brief The default name of components::HttpClient component
    static constexpr std::string_view kName = "http-client";

    HttpClient(const ComponentConfig&, const ComponentContext&);

    clients::http::Client& GetHttpClient();

    static yaml_config::Schema GetStaticConfigSchema();

private:
    clients::http::ClientWithMiddlewares http_client_;
};

template <>
inline constexpr bool kHasValidate<HttpClient> = true;

template <>
inline constexpr auto kConfigFileMode<HttpClient> = ConfigFileMode::kNotRequired;

}  // namespace components

USERVER_NAMESPACE_END
