#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/congestion_control/component.hpp>
#include <userver/server/handlers/server_monitor.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utest/using_namespace_userver.hpp>
#include <userver/utils/daemon_run.hpp>

#include <userver/ugrpc/client/client_factory_component.hpp>
#include <userver/ugrpc/client/component_list.hpp>

#include <greeter_client_tests.hpp>

int main(int argc, const char* const argv[]) {
    const auto component_list = components::MinimalServerComponentList()
                                    .Append<server::handlers::ServerMonitor>()
                                    .Append<congestion_control::Component>()
                                    .Append<components::TestsuiteSupport>()
                                    .Append<server::handlers::TestsControl>()
                                    .Append<clients::dns::Component>()
                                    .Append<components::HttpClient>()
                                    .AppendComponentList(ugrpc::client::DefaultComponentList())
                                    .Append<ugrpc::client::ClientFactoryComponent>()
                                    .Append<functional_tests::ClientComponent>("greeter-client")
                                    .Append<functional_tests::GreeterClientTestComponent>();

    return utils::DaemonMain(argc, argv, component_list);
}
