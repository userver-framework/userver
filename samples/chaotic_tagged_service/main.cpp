#include <userver/chaotic/openapi/middlewares/component_list.hpp>
#include <userver/chaotic/openapi/server/dependencies.hpp>
#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component_list.hpp>
#include <userver/components/component_list.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/daemon_run.hpp>

/// [register]
#include <samples/tagged/greeter/client/component.hpp>
#include <samples/tagged/greeter/handlers/chaotic_handlers_list.hpp>

int main(int argc, char* argv[]) {
    auto component_list =
        USERVER_NAMESPACE::components::MinimalServerComponentList()
            .Append<USERVER_NAMESPACE::components::TestsuiteSupport>()
            .Append<USERVER_NAMESPACE::server::handlers::TestsControl>()
            .AppendComponentList(USERVER_NAMESPACE::clients::http::ComponentList())
            .Append<USERVER_NAMESPACE::clients::dns::Component>()
            .Append<samples::tagged::greeter::client::Component>()
            .Append<USERVER_NAMESPACE::components::Container<
                USERVER_NAMESPACE::chaotic::openapi::server::dependencies::Factories>>()
            .AppendComponentList(samples::tagged::greeter::handlers::ChaoticHandlersList());
    /// [register]

    USERVER_NAMESPACE::chaotic::openapi::middlewares::AppendDefaultMiddlewares(component_list);

    return USERVER_NAMESPACE::utils::DaemonMain(argc, argv, component_list);
}
