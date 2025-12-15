#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component_list.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/daemon_run.hpp>

#include <userver/utest/using_namespace_userver.hpp>

#include <echo_handler.hpp>
#include <echo_handler_detach.hpp>
#include <middleware.hpp>

int main(int argc, char* argv[]) {
    const auto component_list =
        components::MinimalServerComponentList()
            .Append<EchoHandler>()
            .Append<EchoHandlerDetach>()
            .Append<components::TestsuiteSupport>()
            .Append<server::handlers::TestsControl>()
            .Append<clients::dns::Component>()
            .Append<MiddlewareComponent>()
            .AppendComponentList(clients::http::ComponentList());
    return utils::DaemonMain(argc, argv, component_list);
}
