#include <userver/utest/using_namespace_userver.hpp>

#include <userver/components/minimal_server_component_list.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/ugrpc/client/log_middleware/component.hpp>
#include <userver/ugrpc/server/log_middleware/component.hpp>
#include <userver/ugrpc/server/server_component.hpp>
#include <userver/utils/daemon_run.hpp>

#include "handler.hpp"
#include "service.hpp"

int main(int argc, char* argv[]) {
  const auto component_list =
      components::MinimalServerComponentList()
          .Append<components::TestsuiteSupport>()
          .Append<ugrpc::server::ServerComponent>()
          .Append<ugrpc::server::log_middleware::Component>()
          .Append<ugrpc::client::log_middleware::Component>()
          .Append<ugrpc::client::ClientFactoryComponent>()
          .Append<samples::GreeterServiceComponent>()
          .Append<samples::GreeterClient>()
          .Append<samples::GreeterHttpHandler>();
  return utils::DaemonMain(argc, argv, component_list);
}
