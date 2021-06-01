#include <components/common_server_component_list.hpp>

#include <congestion_control/component.hpp>
#include <server/component.hpp>
#include <server/handlers/auth/auth_checker_settings_component.hpp>
#include <server/handlers/implicit_options_http_handler.hpp>
#include <server/handlers/inspect_requests.hpp>
#include <server/handlers/jemalloc.hpp>
#include <server/handlers/log_level.hpp>
#include <server/handlers/server_monitor.hpp>
#include <server/handlers/tests_control.hpp>
#include <server_settings/http_server_settings_component.hpp>

namespace components {

ComponentList CommonServerComponentList() {
  return components::ComponentList()
      .Append<components::Server>()
      .Append<server::handlers::TestsControl>()
      .Append<server::handlers::ServerMonitor>()
      .Append<server::handlers::LogLevel>()
      .Append<server::handlers::InspectRequests>()
      .Append<server::handlers::ImplicitOptionsHttpHandler>()
      .Append<server::handlers::Jemalloc>()
      .Append<congestion_control::Component>()
      .Append<components::AuthCheckerSettings>()
      .Append<components::HttpServerSettings<>>();
}

}  // namespace components
