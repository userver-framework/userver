
#include <userver/utest/using_namespace_userver.hpp>

#include <userver/components/component.hpp>
#include <userver/components/loggable_component_base.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/server_monitor.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/ugrpc/server/middlewares/log/component.hpp>
#include <userver/ugrpc/server/server_component.hpp>
#include <userver/utils/daemon_run.hpp>

#include "my_middleware.hpp"
#include "my_second_middleware.hpp"
#include "service.hpp"

int main(int argc, const char* const argv[]) {
  const auto component_list =
      components::MinimalServerComponentList()
          .Append<server::handlers::ServerMonitor>()
          .Append<components::TestsuiteSupport>()
          .Append<ugrpc::server::ServerComponent>()
          .Append<ugrpc::server::middlewares::log::Component>()
          .Append<functional_tests::MyMiddlewareComponent>()
          .Append<functional_tests::MySecondMiddlewareComponent>()
          .Append<functional_tests::GreeterServiceComponent>();

  return utils::DaemonMain(argc, argv, component_list);
}
