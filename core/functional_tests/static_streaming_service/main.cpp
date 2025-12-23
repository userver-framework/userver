#include <userver/utest/using_namespace_userver.hpp>

#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/http_handler_static_stream.hpp>
#include <userver/utils/daemon_run.hpp>

int main(int argc, char* argv[]) {
    const auto component_list =
        components::MinimalServerComponentList()
            .Append<server::handlers::HttpHandlerStaticStream>();
    return utils::DaemonMain(argc, argv, component_list);
}
