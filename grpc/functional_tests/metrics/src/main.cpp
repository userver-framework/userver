#include <fmt/format.h>

#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/server_monitor.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utest/using_namespace_userver.hpp>
#include <userver/utils/daemon_run.hpp>

#include <userver/ugrpc/client/client_factory_component.hpp>
#include <userver/ugrpc/client/middlewares/baggage/component.hpp>
#include <userver/ugrpc/client/middlewares/deadline_propagation/component.hpp>
#include <userver/ugrpc/client/middlewares/log/component.hpp>
#include <userver/ugrpc/server/middlewares/baggage/component.hpp>
#include <userver/ugrpc/server/middlewares/deadline_propagation/component.hpp>
#include <userver/ugrpc/server/middlewares/log/component.hpp>
#include <userver/ugrpc/server/server_component.hpp>

#include "greeter_client.hpp"
#include "greeter_service.hpp"

int main(int argc, const char* const argv[]) {
  const auto component_list =
      components::MinimalServerComponentList()
          .Append<server::handlers::ServerMonitor>()
          .Append<components::TestsuiteSupport>()
          .Append<ugrpc::server::ServerComponent>()
          .Append<ugrpc::server::middlewares::log::Component>()
          .Append<ugrpc::server::middlewares::deadline_propagation::Component>()
          .Append<ugrpc::server::middlewares::baggage::Component>()
          .Append<ugrpc::client::middlewares::baggage::Component>()
          .Append<ugrpc::client::middlewares::log::Component>()
          .Append<ugrpc::client::middlewares::deadline_propagation::Component>()
          .Append<ugrpc::client::ClientFactoryComponent>()
          .Append<functional_tests::GreeterClient>()
          .Append<functional_tests::GreeterServiceComponent>();

  return utils::DaemonMain(argc, argv, component_list);
}
