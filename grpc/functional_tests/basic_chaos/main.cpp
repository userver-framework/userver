#include <userver/utest/using_namespace_userver.hpp>

#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/congestion_control/component.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/ugrpc/client/client_factory_component.hpp>
#include <userver/ugrpc/client/component_list.hpp>
#include <userver/ugrpc/server/component_list.hpp>
#include <userver/ugrpc/server/middlewares/access_log/component.hpp>
#include <userver/utils/daemon_run.hpp>

#include "handler.hpp"
#include "service.hpp"

int main(int argc, char* argv[]) {
    const auto component_list = components::MinimalServerComponentList()
                                    .Append<components::TestsuiteSupport>()
                                    .AppendComponentList(ugrpc::server::DefaultComponentList())
                                    .Append<ugrpc::server::middlewares::access_log::Component>()
                                    .AppendComponentList(ugrpc::client::DefaultComponentList())
                                    .Append<ugrpc::client::ClientFactoryComponent>()
                                    .Append<server::handlers::TestsControl>()
                                    .Append<components::HttpClient>()
                                    .Append<clients::dns::Component>()
                                    .Append<congestion_control::Component>()
                                    .Append<samples::GreeterServiceComponent>()
                                    .Append<samples::GreeterClient>()
                                    .Append<samples::GreeterClientComponent>("greeter-client-component")
                                    .Append<samples::GreeterHttpHandler>();
    return utils::DaemonMain(argc, argv, component_list);
}
