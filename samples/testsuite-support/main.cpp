#include <userver/utest/using_namespace_userver.hpp>

#include <userver/clients/http/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/ping.hpp>
/// [testsuite - include components]
#include <userver/server/handlers/tests_control.hpp>
#include <userver/testsuite/testsuite_support.hpp>
/// [testsuite - include components]
#include <userver/utils/daemon_run.hpp>

#include "logcapture.hpp"
#include "now.hpp"
#include "testpoint.hpp"

int main(int argc, char* argv[]) {
  const auto component_list =
      // userver components
      components::MinimalServerComponentList()
          .Append<components::HttpClient>()
          .Append<server::handlers::Ping>()
          /// [testsuite - register components]
          .Append<components::TestsuiteSupport>()
          .Append<server::handlers::TestsControl>()
          /// [testsuite - register components]
          // Project local components
          .Append<tests::handlers::Now>()
          .Append<tests::handlers::Testpoint>()
          .Append<tests::handlers::LogCapture>();
  return utils::DaemonMain(argc, argv, component_list);
}
