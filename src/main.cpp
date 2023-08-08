#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/ping.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/daemon_run.hpp>

#include "handlers/profiles/profiles.hpp"
#include "handlers/tags/tags.hpp"
#include "handlers/users/users.hpp"

using namespace real_medium::handlers;

int main(int argc, char* argv[]) {
  auto component_list =
      userver::components::MinimalServerComponentList()
          .Append<userver::server::handlers::Ping>()
          .Append<userver::components::TestsuiteSupport>()
          .Append<userver::components::HttpClient>()
          .Append<userver::components::Postgres>("realmedium-database")
          .Append<userver::clients::dns::Component>()
          .Append<userver::server::handlers::TestsControl>()
          .Append<real_medium::handlers::users::post::RegisterUser>()
          .Append<real_medium::handlers::profiles::get::Handler>()
          .Append<real_medium::handlers::tags::get::Handler>();

  return userver::utils::DaemonMain(argc, argv, component_list);
}
