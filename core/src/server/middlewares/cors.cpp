#include <userver/server/middlewares/cors.hpp>

#include <algorithm>

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/formats/common/merge.hpp>
#include <userver/formats/yaml/serialize.hpp>
#include <userver/formats/yaml/value_builder.hpp>
#include <userver/logging/log.hpp>
#include <userver/server/handlers/exceptions.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/server/http/http_response.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/utils/algo.hpp>
#include <userver/utils/text_light.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#ifndef ARCADIA_ROOT
#include "generated/src/server/middlewares/cors.yaml.hpp"  // Y_IGNORE
#endif

USERVER_NAMESPACE_BEGIN

namespace server::middlewares {

namespace {

constexpr std::string_view kAccessControlRequestMethod = "Access-Control-Request-Method";
constexpr std::string_view kAccessControlAllowMethods = "Access-Control-Allow-Methods";
constexpr std::string_view kAccessControlAllowHeaders = "Access-Control-Allow-Headers";
constexpr std::string_view kAccessControlMaxAge = "Access-Control-Max-Age";
constexpr std::string_view kAccessControlAllowOrigin = "Access-Control-Allow-Origin";
constexpr std::string_view kAccessControlAllowCredentials = "Access-Control-Allow-Credentials";
constexpr std::string_view kAccessControlExposeHeaders = "Access-Control-Expose-Headers";

}  // namespace

Cors::Cors(const Config& config)
    : config_(config)
{}

void Cors::HandleRequest(http::HttpRequest& request, request::RequestContext& context) const {
    const auto& origin = GetOriginHeader(request);

    if (IsPreflightRequest(request)) {
        HandlePreflightRequest(request);
        return;  // Don't call Next() for preflight requests
    }

    // For actual requests, add CORS headers and continue processing
    if (!origin.empty()) {
        if (IsOriginAllowed(origin)) {
            AddCorsHeaders(request, origin);
        } else {
            // NOLINTNEXTLINE(google-build-using-namespace)
            using namespace server::handlers;
            throw ClientError(
                HandlerErrorCode::kUnauthorized,
                ServiceErrorCode{"Access forbidden"},
                InternalMessage{"Origin is forbidden"},
                ExternalBody{"Bad Origin header"}
            );
        }
    }

    Next(request, context);
}

bool Cors::IsPreflightRequest(const http::HttpRequest& request) const {
    return request.GetMethod() == http::HttpMethod::kOptions && request.HasHeader(kAccessControlRequestMethod);
}

void Cors::HandlePreflightRequest(http::HttpRequest& request) const {
    const auto& origin = GetOriginHeader(request);

    if (origin.empty() || !IsOriginAllowed(origin)) {
        request.GetHttpResponse().SetStatus(http::HttpStatus::kForbidden);
        return;
    }

    const auto& requested_method = request.GetHeader(kAccessControlRequestMethod);
    if (requested_method.empty()) {
        request.GetHttpResponse().SetStatus(http::HttpStatus::kBadRequest);
        return;
    }

    if (!utils::Contains(config_.allowed_methods, requested_method)) {
        request.GetHttpResponse().SetStatus(http::HttpStatus::kMethodNotAllowed);
        return;
    }

    // All checks passed, send successful preflight response
    auto& response = request.GetHttpResponse();
    response.SetStatus(http::HttpStatus::kNoContent);

    AddCorsHeaders(request, origin);

    // Add preflight-specific headers
    response.SetHeader(kAccessControlAllowMethods, utils::text::Join(config_.allowed_methods, " ,"));

    if (!config_.allowed_headers.empty()) {
        response.SetHeader(kAccessControlAllowHeaders, utils::text::Join(config_.allowed_headers, ", "));
    }

    response.SetHeader(kAccessControlMaxAge, std::to_string(config_.max_age.count()));
}

void Cors::AddCorsHeaders(http::HttpRequest& request, const std::string& origin) const {
    auto& response = request.GetHttpResponse();

    // Always set the origin for allowed requests
    response.SetHeader(kAccessControlAllowOrigin, origin);

    // Set credentials header if allowed
    if (config_.allow_credentials) {
        response.SetHeader(kAccessControlAllowCredentials, std::string{"true"});
    }

    // Set exposed headers if any
    if (!config_.exposed_headers.empty()) {
        response.SetHeader(kAccessControlExposeHeaders, utils::text::Join(config_.exposed_headers, ", "));
    }

    response.SetHeader(USERVER_NAMESPACE::http::headers::kVary, std::string{"Origin"});
}

bool Cors::IsOriginAllowed(const std::string& origin) const {
    if (origin.empty()) {
        return false;
    }

    // Check if wildcard is allowed
    if (utils::Contains(config_.allowed_origins, "*")) {
        return true;
    }

    LOG_INFO() << config_.allowed_origins;
    // Check for exact matches
    return utils::Contains(config_.allowed_origins, origin);
}

const std::string& Cors::GetOriginHeader(const http::HttpRequest& request) const { return request.GetHeader("Origin"); }

Cors::Config Parse(const yaml_config::YamlConfig& value, formats::parse::To<Cors::Config>) {
    Cors::Config config;

    config.allowed_origins = value["allowed-origins"].As<std::vector<std::string>>();
    config.allow_credentials = value["allow-credentials"].As<bool>(config.allow_credentials);
    config.max_age = std::chrono::seconds(value["max-age-seconds"].As<int>(config.max_age.count()));

    config.allowed_methods = value["allowed-methods"].As<std::vector<std::string>>(config.allowed_methods);
    std::sort(config.allowed_methods.begin(), config.allowed_methods.end());

    config.allowed_headers = value["allowed-headers"].As<std::vector<std::string>>(config.allowed_headers);
    config.exposed_headers = value["exposed-headers"].As<std::vector<std::string>>({});

    return config;
}

CorsFactory::CorsFactory(const components::ComponentConfig& config, const components::ComponentContext& context)
    : HttpMiddlewareFactoryBase{config, context},
      global_config_{(const yaml_config::YamlConfig&)config}  // Explicit slicing
{}

std::unique_ptr<HttpMiddlewareBase> CorsFactory::Create(
    const handlers::HttpHandlerBase&,
    yaml_config::YamlConfig middleware_config
) const {
    formats::yaml::ValueBuilder builder = global_config_.GetRawYamlWithoutConfigVars();
    formats::common::Merge(builder, middleware_config.GetRawYamlWithoutConfigVars());
    yaml_config::YamlConfig config(builder.ExtractValue(), middleware_config.GetRawConfigVars());

    const auto cfg = config.As<Cors::Config>();
    return std::make_unique<Cors>(cfg);
}

yaml_config::Schema CorsFactory::GetStaticConfigSchema() {
    return yaml_config::MergeSchemasFromResource<ComponentBase>("src/server/middlewares/cors.yaml");
}

yaml_config::Schema CorsFactory::GetMiddlewareConfigSchema() const {
    return yaml_config::MergeSchemasFromResource<ComponentBase>("src/server/middlewares/cors.yaml");
}

}  // namespace server::middlewares

USERVER_NAMESPACE_END
