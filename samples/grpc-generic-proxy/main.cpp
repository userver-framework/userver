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
#include <userver/ugrpc/client/client_factory_component.hpp>
#include <userver/ugrpc/client/common_component.hpp>
#include <userver/ugrpc/client/generic.hpp>
#include <userver/ugrpc/client/middlewares/deadline_propagation/component.hpp>
#include <userver/ugrpc/client/middlewares/log/component.hpp>
#include <userver/ugrpc/client/simple_client_component.hpp>
#include <userver/ugrpc/server/middlewares/congestion_control/component.hpp>
#include <userver/ugrpc/server/middlewares/deadline_propagation/component.hpp>
#include <userver/ugrpc/server/middlewares/field_mask_bin/component.hpp>
#include <userver/ugrpc/server/middlewares/log/component.hpp>
#include <userver/ugrpc/server/server_component.hpp>
#include <userver/utils/daemon_run.hpp>

#include <proxy_service.hpp>

int main(int argc, char* argv[]) {
    const auto component_list =
        components::MinimalServerComponentList()
            // Base userver components
            .Append<components::TestsuiteSupport>()
            .Append<congestion_control::Component>()
            // HTTP client and server are (sadly) needed for testsuite support
            .Append<components::HttpClient>()
            .Append<clients::dns::Component>()
            .Append<server::handlers::TestsControl>()
            // gRPC client setup
            .Append<ugrpc::client::CommonComponent>()
            .Append<ugrpc::client::ClientFactoryComponent>()
            .Append<ugrpc::client::middlewares::deadline_propagation::Component>()
            .Append<ugrpc::client::middlewares::log::Component>()
            .Append<ugrpc::client::SimpleClientComponent<ugrpc::client::GenericClient>>("generic-client")
            // gRPC server setup
            .Append<ugrpc::server::ServerComponent>()
            .Append<ugrpc::server::middlewares::congestion_control::Component>()
            .Append<ugrpc::server::middlewares::deadline_propagation::Component>()
            .Append<ugrpc::server::middlewares::log::Component>()
            .Append<ugrpc::server::middlewares::field_mask_bin::Component>()
            .Append<samples::ProxyService>();

    return utils::DaemonMain(argc, argv, component_list);
}
