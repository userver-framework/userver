#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/ping.hpp>
#include <userver/utest/using_namespace_userver.hpp>
#include <userver/utils/daemon_run.hpp>

#include <handler_sigterm.hpp>

int main(int argc, char* argv[]) {
    const auto component_list =
        components::MinimalServerComponentList().Append<server::handlers::Ping>().Append<handlers::Sigterm>();
    return utils::DaemonMain(argc, argv, component_list);
}
