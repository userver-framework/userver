#pragma once

/// [handler policy]
#include <optional>
#include <string>

#include <userver/chaotic/openapi/server/handler_base.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>

namespace samples::chaotic_openapi_service {

// Every generated handler of the target logs and renders errors with it, unless a View says otherwise
struct HandlerPolicy : USERVER_NAMESPACE::chaotic::openapi::server::DefaultHandlerPolicy {
    // Bodies may carry secrets, keep them out of the logs
    static std::optional<std::string> GetRequestBodyForLogging(
        const USERVER_NAMESPACE::server::http::HttpRequest& /*request*/,
        USERVER_NAMESPACE::server::request::RequestContext& /*context*/,
        const std::string& request_body
    ) {
        return "<" + std::to_string(request_body.size()) + " bytes>";
    }

    // Errors the operation does not declare get the service's error envelope
    static std::optional<USERVER_NAMESPACE::server::handlers::FormattedErrorData> GetFormattedExternalErrorBody(
        const USERVER_NAMESPACE::server::handlers::CustomHandlerException& exc
    ) {
        USERVER_NAMESPACE::formats::json::ValueBuilder error;
        error["code"] = exc.GetServiceCode();
        error["message"] = exc.GetExternalErrorBody();
        return USERVER_NAMESPACE::server::handlers::FormattedErrorData{
            USERVER_NAMESPACE::formats::json::ToString(error.ExtractValue()),
            USERVER_NAMESPACE::http::ContentType{"application/json"},
        };
    }
};

}  // namespace samples::chaotic_openapi_service
/// [handler policy]
