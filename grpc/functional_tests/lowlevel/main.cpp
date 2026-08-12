#include <userver/utest/using_namespace_userver.hpp>

#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component_list.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/ugrpc/client/client_factory_component.hpp>
#include <userver/ugrpc/client/component_list.hpp>
#include <userver/utils/daemon_run.hpp>

#include <client_runner.hpp>

int main(int argc, const char* const argv[]) {
    const auto component_list =
        components::MinimalServerComponentList()
            .Append<components::TestsuiteSupport>()
            .AppendComponentList(clients::http::ComponentList())
            .Append<server::handlers::TestsControl>()
            .Append<clients::dns::Component>()
            .Append<ugrpc::client::ClientFactoryComponent>()
            .AppendComponentList(ugrpc::client::MinimalComponentList())
            .Append<ClientRunner>();

    return utils::DaemonMain(argc, argv, component_list);
}
