#include <fmt/format.h>

#include <userver/components/minimal_server_component_list.hpp>
#include <userver/congestion_control/component.hpp>
#include <userver/server/handlers/server_monitor.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utest/using_namespace_userver.hpp>
#include <userver/utils/daemon_run.hpp>

#include <userver/ugrpc/client/client_factory_component.hpp>
#include <userver/ugrpc/client/common_component.hpp>
#include <userver/ugrpc/client/middlewares/baggage/component.hpp>
#include <userver/ugrpc/client/middlewares/deadline_propagation/component.hpp>
#include <userver/ugrpc/client/middlewares/log/component.hpp>
#include <userver/ugrpc/server/component_list.hpp>

#include "greeter_client.hpp"
#include "greeter_service.hpp"

int main(int argc, const char* const argv[]) {
    const auto component_list = components::MinimalServerComponentList()
                                    .Append<server::handlers::ServerMonitor>()
                                    .Append<congestion_control::Component>()
                                    .AppendComponentList(ugrpc::server::DefaultComponentList())
                                    .Append<components::TestsuiteSupport>()
                                    .Append<ugrpc::client::middlewares::baggage::Component>()
                                    .Append<ugrpc::client::middlewares::log::Component>()
                                    .Append<ugrpc::client::middlewares::deadline_propagation::Component>()
                                    .Append<ugrpc::client::CommonComponent>()
                                    .Append<ugrpc::client::ClientFactoryComponent>()
                                    .Append<functional_tests::GreeterClient>()
                                    .Append<functional_tests::GreeterServiceComponent>();

    return utils::DaemonMain(argc, argv, component_list);
}
