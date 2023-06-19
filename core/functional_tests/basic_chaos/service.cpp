#include <userver/clients/dns/component.hpp>
#include <userver/testsuite/testsuite_support.hpp>

#include <userver/components/logging_configurator.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/server_monitor.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/utest/using_namespace_userver.hpp>
#include <userver/utils/daemon_run.hpp>

#include "httpclient_handlers.hpp"
#include "httpserver_handlers.hpp"
#include "resolver_handlers.hpp"

int main(int argc, char* argv[]) {
  const auto component_list = components::MinimalServerComponentList()
                                  .Append<chaos::HttpClientHandler>()
                                  .Append<chaos::StreamHandler>()
                                  .Append<chaos::HttpServerHandler>()
                                  .Append<chaos::ResolverHandler>()
                                  .Append<components::LoggingConfigurator>()
                                  .Append<components::HttpClient>()
                                  .Append<components::TestsuiteSupport>()
                                  .Append<server::handlers::TestsControl>()
                                  .Append<server::handlers::ServerMonitor>()
                                  .Append<clients::dns::Component>();
  return utils::DaemonMain(argc, argv, component_list);
}
