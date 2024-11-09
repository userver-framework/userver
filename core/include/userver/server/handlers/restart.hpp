#pragma once

/// @file userver/server/handlers/restart.hpp
/// @brief @copybrief server::handlers::Restart

#include <atomic>

#include <userver/server/handlers/http_handler_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers {

// clang-format off

/// @ingroup userver_components userver_http_handlers
///
/// @brief Handler that stops the service. It is expected that an external system
/// (e.g. systemd or supervisord) restarts the service afterwards.
///
/// The handler uses monitor port.
///
/// The component has no service configuration except the
/// @ref userver_http_handlers "common handler options".

// clang-format on

class Restart final : public HttpHandlerBase {
public:
    Restart(const components::ComponentConfig&, const components::ComponentContext&);

    /// @ingroup userver_component_names
    /// @brief The default name of server::handlers::Restart
    static constexpr std::string_view kName = "handler-restart";

    std::string HandleRequestThrow(const http::HttpRequest&, request::RequestContext&) const override;

    components::ComponentHealth GetComponentHealth() const override;

private:
    mutable std::atomic<components::ComponentHealth> health_;
};

}  // namespace server::handlers

USERVER_NAMESPACE_END
