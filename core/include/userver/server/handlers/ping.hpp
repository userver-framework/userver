#pragma once

/// @file userver/server/handlers/ping.hpp
/// @brief @copybrief server::handlers::Ping

#include <userver/components/state.hpp>
#include <userver/server/handlers/http_handler_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers {

/// @ingroup userver_components userver_http_handlers
///
/// @brief Base class for handlers that returns HTTP 200 if the service
/// is OK and able to process requests.
///
/// Uses components::State::IsAnyComponentInFatalState() to detect
/// fatal state (can not process requests).
///
/// ## Static options of server::handlers::PingBase:
///
/// Options inherited from @ref server::handlers::HttpHandlerBase :
/// @include{doc} scripts/docs/en/components_schema/core/src/server/handlers/http_handler_base.md
///
/// Options inherited from @ref server::handlers::HandlerBase :
/// @include{doc} scripts/docs/en/components_schema/core/src/server/handlers/handler_base.md
///
/// Options inherited from @ref components::ComponentBase :
/// @include{doc} scripts/docs/en/components_schema/core/src/components/impl/component_base.md
///
/// @ref userver_http_handlers
class PingBase : public HttpHandlerBase {
public:
    PingBase(const components::ComponentConfig& config, const components::ComponentContext& component_context);

    std::string HandleRequestThrow(const http::HttpRequest& request, request::RequestContext& context) const override;

private:
    const components::State components_;
};

/// @ingroup userver_components userver_http_handlers
///
/// @brief Ping handler implementation with warmup
///
/// ## Static options of server::handlers::Ping:
/// @include{doc} scripts/docs/en/components_schema/ccore/src/server/handlers/ping.md
///
/// Options inherited from @ref server::handlers::HandlerBase :
/// @include{doc} scripts/docs/en/components_schema/core/src/server/handlers/handler_base.md
///
/// Options inherited from @ref components::ComponentBase :
/// @include{doc} scripts/docs/en/components_schema/core/src/components/impl/component_base.md
class Ping final : public PingBase {
public:
    Ping(const components::ComponentConfig& config, const components::ComponentContext& component_context);

    /// @ingroup userver_component_names
    /// @brief The default name of server::handlers::Ping
    static constexpr std::string_view kName = "handler-ping";

    std::string HandleRequestThrow(const http::HttpRequest& request, request::RequestContext& context) const override;

    void OnAllComponentsLoaded() override;

    static yaml_config::Schema GetStaticConfigSchema();

private:
    void AppendWeightHeaders(http::HttpResponse&) const;

    std::chrono::steady_clock::time_point load_time_{};
    std::chrono::seconds awacs_weight_warmup_time_{60};
};

}  // namespace server::handlers

template <>
inline constexpr bool components::kHasValidate<server::handlers::Ping> = true;

USERVER_NAMESPACE_END
