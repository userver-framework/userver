#pragma once

/// @file userver/server/handlers/http_handler_json_base.hpp
/// @brief @copybrief server::handlers::HttpHandlerJsonBase

#include <userver/server/handlers/http_handler_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers {

// clang-format off

/// @ingroup userver_components userver_http_handlers userver_base_classes
///
/// @brief Convenient base for handlers that accept requests with body in
/// JSON format and respond with body in JSON format.
///
/// ## Example usage:
///
/// @snippet samples/config_service/main.cpp Config service sample - component

// clang-format on

class HttpHandlerJsonBase : public HttpHandlerBase {
public:
    using Value = formats::json::Value;
    using HttpRequest = server::http::HttpRequest;
    using RequestContext = server::request::RequestContext;

    HttpHandlerJsonBase(
        const components::ComponentConfig& config,
        const components::ComponentContext& component_context,
        bool is_monitor = false
    );

    std::string HandleRequestThrow(const http::HttpRequest& request, request::RequestContext& context) const final;

    virtual Value HandleRequestJsonThrow(const HttpRequest& request, const Value& request_json, RequestContext& context)
        const = 0;

    static yaml_config::Schema GetStaticConfigSchema();

protected:
    /// @returns A pointer to json request if it was parsed successfully or
    /// nullptr otherwise.
    static const formats::json::Value* GetRequestJson(const request::RequestContext& context);

    /// @returns a pointer to json response if it was returned successfully by
    /// `HandleRequestJsonThrow()` or nullptr otherwise.
    static const formats::json::Value* GetResponseJson(const request::RequestContext& context);

    void ParseRequestData(const http::HttpRequest& request, request::RequestContext& context) const override;

private:
    FormattedErrorData GetFormattedExternalErrorBody(const CustomHandlerException& exc) const final;
};

}  // namespace server::handlers

template <>
inline constexpr bool components::kHasValidate<server::handlers::HttpHandlerJsonBase> = true;

USERVER_NAMESPACE_END
