#include <userver/clients/dns/component.hpp>
#include <userver/testsuite/testsuite_support.hpp>

#include <userver/alerts/handler.hpp>
#include <userver/components/logging_configurator.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/dynamic_config/client/component.hpp>
#include <userver/dynamic_config/updater/component.hpp>
#include <userver/server/handlers/on_log_rotate.hpp>
#include <userver/server/handlers/ping.hpp>
#include <userver/server/handlers/server_monitor.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/utest/using_namespace_userver.hpp>
#include <userver/utils/daemon_run.hpp>

#include "httpclient_handlers.hpp"
#include "httpserver_handlers.hpp"
#include "resolver_handlers.hpp"

int main(int argc, char* argv[]) {
  const auto component_list =
      components::MinimalServerComponentList()
          .Append<chaos::HttpClientHandler>()
          .Append<chaos::StreamHandler>()
          .Append<chaos::HttpServerHandler>()
          .Append<chaos::ResolverHandler>()
          .Append<components::LoggingConfigurator>()
          .Append<components::HttpClient>()
          .Append<components::TestsuiteSupport>()
          .Append<server::handlers::TestsControl>()
          .Append<server::handlers::ServerMonitor>()
          .Append<server::handlers::Ping>()
          .Append<clients::dns::Component>()
          .Append<alerts::Handler>()
          .Append<components::DynamicConfigClient>()
          .Append<components::DynamicConfigClientUpdater>()
          .Append<server::handlers::OnLogRotate>();

  return utils::DaemonMain(argc, argv, component_list);
}
