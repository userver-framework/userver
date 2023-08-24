#include <userver/components/common_server_component_list.hpp>

#include <server/handlers/implicit_options_http_handler.hpp>
#include <userver/congestion_control/component.hpp>
#include <userver/server/component.hpp>
#include <userver/server/handlers/auth/auth_checker_settings_component.hpp>
#include <userver/server/handlers/dns_client_control.hpp>
#include <userver/server/handlers/dynamic_debug_log.hpp>
#include <userver/server/handlers/inspect_requests.hpp>
#include <userver/server/handlers/jemalloc.hpp>
#include <userver/server/handlers/log_level.hpp>
#include <userver/server/handlers/on_log_rotate.hpp>
#include <userver/server/handlers/server_monitor.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/tracing/manager_component.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

ComponentList CommonServerComponentList() {
  return components::ComponentList()
      .Append<components::Server>()
      .Append<server::handlers::DnsClientControl>()
      .Append<server::handlers::DynamicDebugLog>()
      .Append<server::handlers::ImplicitOptionsHttpHandler>()
      .Append<server::handlers::InspectRequests>()
      .Append<server::handlers::Jemalloc>()
      .Append<server::handlers::LogLevel>()
      .Append<server::handlers::OnLogRotate>()
      .Append<server::handlers::ServerMonitor>()
      .Append<server::handlers::TestsControl>()
      .Append<tracing::DefaultTracingManagerLocator>()
      .Append<congestion_control::Component>()
      .Append<components::AuthCheckerSettings>();
}

}  // namespace components

USERVER_NAMESPACE_END
