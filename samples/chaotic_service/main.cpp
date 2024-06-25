#include <userver/utest/using_namespace_userver.hpp>

#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/components/common_component_list.hpp>
#include <userver/components/common_server_component_list.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/daemon_run.hpp>

#include "hello_service.hpp"

int main(int argc, char* argv[]) {
  auto component_list = components::MinimalServerComponentList()
                            .Append<components::TestsuiteSupport>()
                            .Append<server::handlers::TestsControl>()
                            .Append<clients::dns::Component>()
                            .Append<components::HttpClient>();
  samples::hello::AppendHello(component_list);

  return utils::DaemonMain(argc, argv, component_list);
}
