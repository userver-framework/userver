#pragma once

/// @file userver/server/handlers/handler_base.hpp
/// @brief @copybrief server::handlers::HandlerBase

#include <userver/components/component_base.hpp>
#include <userver/server/handlers/exceptions.hpp>
#include <userver/server/handlers/handler_config.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/server/request/request_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers {

/// @brief Base class for the request handlers.
///
/// Each handler has an associated path and invoked only for the requests for that path.
///
/// ## Static options of server::handlers::HandlerBase :
/// @include{doc} scripts/docs/en/components_schema/core/src/server/handlers/handler_base.md
///
/// Options inherited from @ref components::ComponentBase :
/// @include{doc} scripts/docs/en/components_schema/core/src/components/impl/component_base.md
class HandlerBase : public components::ComponentBase {
public:
    HandlerBase(
        const components::ComponentConfig& config,
        const components::ComponentContext& component_context,
        bool is_monitor = false
    );
    ~HandlerBase() noexcept override = default;

    /// Parses request, executes processing routines, and fills response
    /// accordingly. Does not throw.
    virtual void PrepareAndHandleRequest(http::HttpRequest& request, request::RequestContext& context) const = 0;

    /// Produces response to a request unrecognized by the protocol based on
    /// provided generic response. Does not throw.
    /// The error should be stored into response status and body prior to this call.
    virtual void ReportMalformedRequest(http::HttpRequest&) const {}

    /// Returns whether this is a monitoring handler.
    bool IsMonitor() const { return is_monitor_; }

    /// Returns handler config.
    const HandlerConfig& GetConfig() const;

    static yaml_config::Schema GetStaticConfigSchema();

protected:
    // Pull the type names in the handler's scope to shorten throwing code
    using HandlerErrorCode = handlers::HandlerErrorCode;
    using InternalMessage = handlers::InternalMessage;
    using ExternalBody = handlers::ExternalBody;

    using ClientError = handlers::ClientError;
    using InternalServerError = handlers::InternalServerError;

private:
    bool is_monitor_;
    HandlerConfig config_;
};

}  // namespace server::handlers

USERVER_NAMESPACE_END
