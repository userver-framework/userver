#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/ping.hpp>
#include <userver/server/middlewares/cors.hpp>
#include <userver/utest/using_namespace_userver.hpp>
#include <userver/utils/daemon_run.hpp>

// Note: this is for the purposes of tests/samples only
#include <userver/utest/using_namespace_userver.hpp>

int main(int argc, char* argv[]) {
    auto component_list =
        components::MinimalServerComponentList()
            .Append<server::middlewares::CorsFactory>()
            .Append<server::handlers::Ping>();

    return utils::DaemonMain(argc, argv, component_list);
}
