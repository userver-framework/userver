#include <userver/utest/using_namespace_userver.hpp>

#include <userver/components/component.hpp>
#include <userver/components/loggable_component_base.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/handlers/ping.hpp>
#include <userver/server/handlers/server_monitor.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/daemon_run.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <userver/ugrpc/client/client_factory_component.hpp>

#include <userver/otlp/logs/component.hpp>

int main(int argc, char* argv[]) {
  const auto component_list =
      components::MinimalServerComponentList()
          .Append<components::TestsuiteSupport>()
          .Append<server::handlers::ServerMonitor>()
          .Append<ugrpc::client::ClientFactoryComponent>()
          .Append<server::handlers::Ping>()
          .Append<otlp::LoggerComponent>();
  return utils::DaemonMain(argc, argv, component_list);
}
