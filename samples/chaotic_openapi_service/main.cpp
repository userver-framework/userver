#include <userver/chaotic/openapi/middlewares/component_list.hpp>
#include <userver/chaotic/openapi/middlewares/qos_middleware.hpp>
#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/dynamic_config/client/component.hpp>
#include <userver/dynamic_config/updater/component.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utest/using_namespace_userver.hpp>
#include <userver/utils/daemon_run.hpp>

#include <clients/test/component.hpp>
#include <clients/test/qos.hpp>

#include <hello_handler.hpp>

int main(int argc, char* argv[]) {
    auto component_list =
        USERVER_NAMESPACE::components::MinimalServerComponentList()
            .Append<samples::hello::HelloHandler>()
            .Append<USERVER_NAMESPACE::components::DynamicConfigClient>()
            .Append<USERVER_NAMESPACE::components::TestsuiteSupport>()
            .Append<USERVER_NAMESPACE::server::handlers::TestsControl>()
            .Append<USERVER_NAMESPACE::components::DynamicConfigClientUpdater>()
            .Append<USERVER_NAMESPACE::components::HttpClient>()
            .Append<USERVER_NAMESPACE::clients::dns::Component>()
            .Append<USERVER_NAMESPACE::chaotic::openapi::QosMiddlewareFactory<::clients::test::kQosConfig>>(
                "chaotic-client-middleware-qos-test"
            )
            .Append<::clients::test::Component>();

    USERVER_NAMESPACE::chaotic::openapi::middlewares::AppendDefaultMiddlewares(component_list);

    return USERVER_NAMESPACE::utils::DaemonMain(argc, argv, component_list);
}
