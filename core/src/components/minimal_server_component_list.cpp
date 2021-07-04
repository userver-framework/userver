#include <userver/components/minimal_component_list.hpp>
#include <userver/components/minimal_server_component_list.hpp>

#include <userver/server/component.hpp>
#include <userver/server/handlers/auth/auth_checker_settings_component.hpp>

namespace components {

ComponentList MinimalServerComponentList() {
  return components::MinimalComponentList()
      .Append<components::Server>()
      .Append<components::AuthCheckerSettings>();
}

}  // namespace components
