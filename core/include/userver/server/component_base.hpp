#pragma once

#include <userver/components/component_base.hpp>
#include <userver/server/server_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace server {

/// Component integration for HTTP servers registered under the name "server".
/// Concrete components own their server and forward component lifecycle callbacks.
/// The current implementation is @ref components::Server.
class Component : public components::ComponentBase {
public:
    /// @ingroup userver_component_names
    /// @brief The default name of the HTTP server component.
    static constexpr std::string_view kName = "server";

    /// @brief Return the HTTP server runtime owned by this component.
    virtual ServerBase& GetServer() = 0;

    /// @brief Return the HTTP server runtime owned by this component.
    virtual const ServerBase& GetServer() const = 0;

protected:
    using components::ComponentBase::ComponentBase;
};

}  // namespace server

USERVER_NAMESPACE_END
