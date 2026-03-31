#include <userver/utest/using_namespace_userver.hpp>

#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component_list.hpp>
#include <userver/components/minimal_component_list.hpp>
#include <userver/server/component.hpp>
#include <userver/server/handlers/auth/auth_checker_settings_component.hpp>
#include <userver/server/handlers/ping.hpp>
#include <userver/server/handlers/server_monitor.hpp>
#include <userver/server/middlewares/configuration.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/daemon_run.hpp>

#include <handler.hpp>

int main(int argc, const char* const argv[]) {
    auto component_list =
        components::MinimalComponentList()
            .Append<components::Server>()
            .Append<components::TestsuiteSupport>()
            .Append<components::AuthCheckerSettings>()
            .Append<clients::dns::Component>()
            .Append<server::handlers::ServerMonitor>()
            .Append<server::handlers::Ping>()
            .Append<handler::HandlerVeryImportantProduct>()
            .AppendComponentList(clients::http::ComponentList())
            .AppendComponentList(server::middlewares::DefaultMiddlewareComponents());

    return utils::DaemonMain(argc, argv, component_list);
}
