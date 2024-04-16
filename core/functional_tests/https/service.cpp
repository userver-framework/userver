#include <userver/clients/dns/component.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/storages/secdist/provider_component.hpp>
#include <userver/testsuite/testsuite_support.hpp>

#include <userver/alerts/handler.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/components/logging_configurator.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/dynamic_config/client/component.hpp>
#include <userver/dynamic_config/updater/component.hpp>
#include <userver/server/handlers/on_log_rotate.hpp>
#include <userver/server/handlers/server_monitor.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/utest/using_namespace_userver.hpp>
#include <userver/utils/daemon_run.hpp>

#include "httpserver_handlers.hpp"

int main(int argc, char* argv[]) {
  const auto component_list = components::MinimalServerComponentList()
                                  .Append<https::HttpServerHandler>()
                                  .Append<components::DefaultSecdistProvider>()
                                  .Append<components::Secdist>()
                                  .Append<components::LoggingConfigurator>()
                                  .Append<components::HttpClient>()
                                  .Append<components::TestsuiteSupport>()
                                  .Append<server::handlers::TestsControl>()
                                  .Append<server::handlers::ServerMonitor>()
                                  .Append<clients::dns::Component>()
                                  .Append<alerts::Handler>()
                                  .Append<server::handlers::OnLogRotate>();

  return utils::DaemonMain(argc, argv, component_list);
}
