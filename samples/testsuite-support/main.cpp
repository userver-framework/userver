#include <userver/utest/using_namespace_userver.hpp>

#include <userver/clients/http/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/ping.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/daemon_run.hpp>

#include "now.hpp"
#include "testpoint.hpp"

int main(int argc, char* argv[]) {
  const auto component_list =
      // userver components
      components::MinimalServerComponentList()
          .Append<components::HttpClient>()
          .Append<userver::server::handlers::Ping>()
          .Append<components::TestsuiteSupport>()
          .Append<server::handlers::TestsControl>()
          // Project local components
          .Append<tests::handlers::Now>()
          .Append<tests::handlers::Testpoint>();
  return utils::DaemonMain(argc, argv, component_list);
}
