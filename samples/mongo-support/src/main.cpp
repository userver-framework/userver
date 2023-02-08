#include <userver/utest/using_namespace_userver.hpp>

#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/congestion_control/component.hpp>
#include <userver/server/handlers/ping.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/storages/mongo/component.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/daemon_run.hpp>

#include "distlock_worker.hpp"

int main(int argc, char* argv[]) {
  const auto component_list =
      // userver components
      components::MinimalServerComponentList()
          .Append<clients::dns::Component>()
          .Append<components::HttpClient>()
          .Append<congestion_control::Component>()
          .Append<server::handlers::Ping>()
          .Append<server::handlers::TestsControl>()
          .Append<components::TestsuiteSupport>()
          .Append<components::Mongo>("mongo-sample")
          // Project local components
          .Append<tests::distlock::MongoWorkerComponent>();
  return utils::DaemonMain(argc, argv, component_list);
}
