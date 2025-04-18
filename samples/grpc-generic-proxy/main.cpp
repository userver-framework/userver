// For testing purposes only, in your services write out userver:: namespace
// instead.
#include <userver/utest/using_namespace_userver.hpp>

#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/congestion_control/component.hpp>
#include <userver/server/component.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/daemon_run.hpp>

#include <userver/ugrpc/client/client_factory_component.hpp>
#include <userver/ugrpc/client/common_component.hpp>
#include <userver/ugrpc/client/component_list.hpp>
#include <userver/ugrpc/client/generic_client.hpp>
#include <userver/ugrpc/client/middlewares/deadline_propagation/component.hpp>
#include <userver/ugrpc/client/middlewares/log/component.hpp>
#include <userver/ugrpc/client/simple_client_component.hpp>
#include <userver/ugrpc/server/component_list.hpp>

#include <proxy_service.hpp>

int main(int argc, char* argv[]) {
    const auto component_list =
        components::MinimalServerComponentList()
            // Base userver components
            .Append<congestion_control::Component>()
            .Append<components::TestsuiteSupport>()
            // HTTP client and server are (sadly) needed for testsuite support
            .Append<components::HttpClient>()
            .Append<clients::dns::Component>()
            .Append<server::handlers::TestsControl>()
            // gRPC client setup
            .AppendComponentList(ugrpc::client::MinimalComponentList())
            .Append<ugrpc::client::ClientFactoryComponent>()
            .Append<ugrpc::client::SimpleClientComponent<ugrpc::client::GenericClient>>("generic-client")
            // gRPC server setup
            .AppendComponentList(ugrpc::server::MinimalComponentList())
            .Append<samples::ProxyService>();

    return utils::DaemonMain(argc, argv, component_list);
}
