#pragma once

#ifdef USERVER_TVM2_HTTP_CLIENT
#error Use components::Http from clients/http.hpp instead
#endif

/// @file userver/clients/http/component.hpp
/// @brief @copybrief components::HttpClient

#include <memory>

#include <userver/clients/http/client.hpp>
#include <userver/clients/http/client_with_middlewares.hpp>
#include <userver/clients/http/component_core.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

/// @ingroup userver_components
///
/// @brief Component exposing the configured @ref clients::http::Client.
///
/// The default implementation builds a @ref clients::http::ClientWithMiddlewares
/// using @ref components::HttpClientCore. A derived component can supply its own
/// client and static configuration schema while retaining the same lookup type.
///
/// Returned references to @ref clients::http::Client live for a lifetime of the
/// component and are safe for concurrent use.
///
/// Embedded username/password in request and redirect URLs are rejected by
/// default. Use clients::http::Request::http_auth_type for HTTP authentication.
/// For integrations that require credentials in URLs, set
/// `allow-url-userinfo: true` or enable
/// clients::http::Request::SetUrlUserinfoAllowed on the request.
/// Credentials in proxy URLs remain supported.
///
/// ## Static options of components::HttpClient :
/// @include{doc} scripts/docs/en/components_schema/core/src/clients/http/component.md
///
/// Options inherited from @ref components::ComponentBase :
/// @include{doc} scripts/docs/en/components_schema/core/src/components/impl/component_base.md
///
/// ## Static configuration example:
///
/// @snippet core/src/components/common_component_list_test.cpp  Sample http client component config
class HttpClient : public ComponentBase {
public:
    /// @ingroup userver_component_names
    /// @brief The default name of components::HttpClient component
    static constexpr std::string_view kName = "http-client";

    /// @ingroup userver_component_names
    /// @brief Name of components::HttpClient component used for obtaining dynamic configs
    static constexpr std::string_view kDynamicConfigClientName = "dynamic-config-http-client";

    HttpClient(const ComponentConfig&, const ComponentContext&);

    clients::http::Client& GetHttpClient();

    static yaml_config::Schema GetStaticConfigSchema();

protected:
    /// Construct the component with an alternative client implementation.
    HttpClient(const ComponentConfig&, const ComponentContext&, std::unique_ptr<clients::http::Client>);

private:
    std::unique_ptr<clients::http::Client> http_client_;
};

template <>
inline constexpr bool kHasValidate<HttpClient> = true;

template <>
inline constexpr auto kConfigFileMode<HttpClient> = ConfigFileMode::kNotRequired;

}  // namespace components

USERVER_NAMESPACE_END
