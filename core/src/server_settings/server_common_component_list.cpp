#include <server_settings/server_common_component_list.hpp>

#include <server/component.hpp>
#include <server/handlers/auth/auth_checker_settings_component.hpp>
#include <server/handlers/inspect_requests.hpp>
#include <server/handlers/server_monitor.hpp>
#include <server/handlers/tests_control.hpp>

namespace server_settings {
namespace impl {

components::ComponentList ServerCommonComponentList() {
  return components::ComponentList()
      .Append<components::Server>()
      .Append<server::handlers::TestsControl>()
      .Append<server::handlers::ServerMonitor>()
      .Append<server::handlers::InspectRequests>()
      .Append<components::AuthCheckerSettings>();
}

}  // namespace impl
}  // namespace server_settings
