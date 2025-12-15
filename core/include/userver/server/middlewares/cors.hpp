#pragma once

/// @file userver/server/middlewares/cors.hpp
/// @brief CORS (Cross-Origin Resource Sharing) middleware

#include <string>
#include <vector>

#include <userver/formats/parse/to.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/server/http/http_method.hpp>
#include <userver/server/middlewares/builtin.hpp>
#include <userver/server/middlewares/http_middleware_base.hpp>
#include <userver/yaml_config/schema.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::middlewares {

/// @brief CORS (Cross-Origin Resource Sharing) middleware
///
/// This middleware handles CORS preflight requests and adds appropriate CORS headers
/// to responses to allow cross-origin requests from web browsers.
///
/// @note The middleware is usually created by @ref server::middlewares::CorsFactory
class Cors final : public HttpMiddlewareBase {
public:
    struct Config {
        /// Allowed origins. Use "*" to allow all origins (not recommended for production)
        std::vector<std::string> allowed_origins;

        /// Allowed HTTP methods
        std::vector<std::string> allowed_methods = {
            ToString(http::HttpMethod::kGet),
            ToString(http::HttpMethod::kPost),
            ToString(http::HttpMethod::kPut),
            ToString(http::HttpMethod::kPatch),
            ToString(http::HttpMethod::kDelete),
            ToString(http::HttpMethod::kHead),
            ToString(http::HttpMethod::kOptions),
        };

        /// Allowed headers
        std::vector<std::string> allowed_headers = {
            std::string(USERVER_NAMESPACE::http::headers::kAccept),
            std::string(USERVER_NAMESPACE::http::headers::kAcceptLanguage),
            std::string(USERVER_NAMESPACE::http::headers::kContentLanguage),
            std::string(USERVER_NAMESPACE::http::headers::kContentType),
        };

        /// Headers that can be exposed to the browser
        std::vector<std::string> exposed_headers;

        /// Whether to allow credentials (cookies, authorization headers)
        bool allow_credentials{false};

        /// Maximum age for preflight cache
        std::chrono::seconds max_age{std::chrono::hours(24)};
    };

    explicit Cors(const Config& config);

private:
    void HandleRequest(http::HttpRequest& request, request::RequestContext& context) const override;

    bool IsPreflightRequest(const http::HttpRequest& request) const;
    void HandlePreflightRequest(http::HttpRequest& request) const;
    void AddCorsHeaders(http::HttpRequest& request, const std::string& origin) const;
    bool IsOriginAllowed(const std::string& origin) const;
    const std::string& GetOriginHeader(const http::HttpRequest& request) const;

    const Config config_;
};

/// @ingroup userver_components
///
/// @brief Factory for @ref server::middlewares::Cors
///
/// The middleware handles CORS preflight requests and adds appropriate CORS headers
/// to responses to allow cross-origin requests from web browsers.
/// For additional information about CORS please see https://developer.mozilla.org/en-US/docs/Web/HTTP/Guides/CORS.
///
/// ## Static options of `CorsFactory`:
///
/// @include{doc} scripts/docs/en/components_schema/core/src/server/middlewares/cors.md
///
/// Options inherited from @ref components::RawComponentBase :
/// @include{doc} scripts/docs/en/components_schema/core/src/components/impl/component_base.md
///
/// @see @ref server::middlewares::Cors
class CorsFactory final : public HttpMiddlewareFactoryBase {
public:
    /// @ingroup userver_component_names
    /// @brief The default name of @ref server::middlewares::CorsFactory component
    static constexpr std::string_view kName = "cors-middleware";

    CorsFactory(const components::ComponentConfig&, const components::ComponentContext&);

    static yaml_config::Schema GetStaticConfigSchema();

    yaml_config::Schema GetMiddlewareConfigSchema() const override;

private:
    std::unique_ptr<HttpMiddlewareBase> Create(
        const handlers::HttpHandlerBase& handler,
        yaml_config::YamlConfig middleware_config
    ) const override;

    const yaml_config::YamlConfig global_config_;
};

/// Parse CORS configuration from YAML
Cors::Config Parse(const yaml_config::YamlConfig& value, formats::parse::To<Cors::Config>);

}  // namespace server::middlewares

template <>
inline constexpr bool components::kHasValidate<server::middlewares::CorsFactory> = true;

template <>
inline constexpr auto components::kConfigFileMode<server::middlewares::CorsFactory> = ConfigFileMode::kRequired;

USERVER_NAMESPACE_END
