#include <userver/utest/using_namespace_userver.hpp>

#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/congestion_control/component.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/ugrpc/client/common_component.hpp>
#include <userver/ugrpc/client/middlewares/baggage/component.hpp>
#include <userver/ugrpc/client/middlewares/deadline_propagation/component.hpp>
#include <userver/ugrpc/client/middlewares/log/component.hpp>
#include <userver/ugrpc/server/middlewares/baggage/component.hpp>
#include <userver/ugrpc/server/middlewares/congestion_control/component.hpp>
#include <userver/ugrpc/server/middlewares/deadline_propagation/component.hpp>
#include <userver/ugrpc/server/middlewares/field_mask/component.hpp>
#include <userver/ugrpc/server/middlewares/log/component.hpp>
#include <userver/ugrpc/server/server_component.hpp>
#include <userver/utils/daemon_run.hpp>

#include "handler.hpp"
#include "service.hpp"

int main(int argc, char* argv[]) {
    const auto component_list = components::MinimalServerComponentList()
                                    .Append<components::TestsuiteSupport>()
                                    .Append<ugrpc::server::ServerComponent>()
                                    .Append<ugrpc::server::middlewares::baggage::Component>()
                                    .Append<ugrpc::server::middlewares::log::Component>()
                                    .Append<ugrpc::server::middlewares::deadline_propagation::Component>()
                                    .Append<ugrpc::server::middlewares::congestion_control::Component>()
                                    .Append<ugrpc::server::middlewares::field_mask::Component>()
                                    .Append<ugrpc::client::middlewares::baggage::Component>()
                                    .Append<ugrpc::client::middlewares::log::Component>()
                                    .Append<ugrpc::client::middlewares::deadline_propagation::Component>()
                                    .Append<ugrpc::client::CommonComponent>()
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
