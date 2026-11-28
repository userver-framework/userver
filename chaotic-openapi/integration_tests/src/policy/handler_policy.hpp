#pragma once

#include <optional>
#include <string>

#include <userver/chaotic/openapi/server/handler_base.hpp>

namespace test::policy {

// Logs no bodies and renders undeclared errors as {"error": "<message>"}
struct HandlerPolicy : USERVER_NAMESPACE::chaotic::openapi::server::DefaultHandlerPolicy {
    static std::optional<std::string> GetRequestBodyForLogging(
        const USERVER_NAMESPACE::server::http::HttpRequest& /*request*/,
        USERVER_NAMESPACE::server::request::RequestContext& /*context*/,
        const std::string& /*request_body*/
    ) {
        return "<request redacted by policy>";
    }

    static std::optional<std::string> GetResponseDataForLogging(
        const USERVER_NAMESPACE::server::http::HttpRequest& /*request*/,
        USERVER_NAMESPACE::server::request::RequestContext& /*context*/,
        const std::string& /*response_data*/
    ) {
        return "<response redacted by policy>";
    }

    static std::optional<USERVER_NAMESPACE::server::handlers::FormattedErrorData> GetFormattedExternalErrorBody(
        const USERVER_NAMESPACE::server::handlers::CustomHandlerException& exc
    ) {
        return USERVER_NAMESPACE::server::handlers::FormattedErrorData{
            R"({"error":")" + exc.GetExternalErrorBody() + R"("})",
            USERVER_NAMESPACE::http::ContentType{"application/json"},
        };
    }
};

}  // namespace test::policy
