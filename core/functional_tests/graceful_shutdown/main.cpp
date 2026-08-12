#include "delaying_handler.hpp"

#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component_list.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/dynamic_config/updater/component_list.hpp>
#include <userver/server/handlers/log_level.hpp>
#include <userver/server/handlers/ping.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utest/using_namespace_userver.hpp>
#include <userver/utils/daemon_run.hpp>

int main(int argc, char* argv[]) {
    const auto component_list =
        components::MinimalServerComponentList()
            .AppendComponentList(USERVER_NAMESPACE::dynamic_config::updater::ComponentList())
            .AppendComponentList(clients::http::ComponentList())
            .Append<clients::dns::Component>()
            .Append<server::handlers::TestsControl>()
            .Append<components::TestsuiteSupport>()
            .Append<server::handlers::Ping>()
            .Append<server::handlers::testing::DelayingServerHandler>();
    return utils::DaemonMain(argc, argv, component_list);
}
