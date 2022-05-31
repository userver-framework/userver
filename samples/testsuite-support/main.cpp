#include <userver/utest/using_namespace_userver.hpp>

#include <userver/clients/http/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/congestion_control/component.hpp>
#include <userver/server/handlers/ping.hpp>
/// [testsuite - include components]
#include <userver/server/handlers/tests_control.hpp>
#include <userver/testsuite/testsuite_support.hpp>
/// [testsuite - include components]
#include <userver/server/handlers/server_monitor.hpp>
#include <userver/utils/daemon_run.hpp>

#include "logcapture.hpp"
#include "metrics.hpp"
#include "now.hpp"
#include "tasks.hpp"
#include "testpoint.hpp"

int main(int argc, char* argv[]) {
  const auto component_list =
      // userver components
      components::MinimalServerComponentList()
          .Append<components::HttpClient>()
          .Append<congestion_control::Component>()
          .Append<server::handlers::Ping>()
          .Append<server::handlers::ServerMonitor>()
          /// [testsuite - register components]
          .Append<components::TestsuiteSupport>()
          .Append<server::handlers::TestsControl>()
          /// [testsuite - register components]
          // Project local components
          .Append<tests::handlers::LogCapture>()
          .Append<tests::handlers::Metrics>()
          .Append<tests::handlers::Now>()
          .Append<tests::handlers::TasksSample>()
          .Append<tests::handlers::Testpoint>();
  return utils::DaemonMain(argc, argv, component_list);
}
