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

// Note: this is for the purposes of tests/samples only
#include <userver/utest/using_namespace_userver.hpp>

#include <clients/test/component.hpp>
#include <clients/test/qos.hpp>

#include <hello_handler.hpp>

int main(int argc, char* argv[]) {
    auto component_list = components::MinimalServerComponentList()
                              .Append<samples::hello::HelloHandler>()
                              .Append<components::DynamicConfigClient>()
                              .Append<components::TestsuiteSupport>()
                              .Append<server::handlers::TestsControl>()
                              .Append<components::DynamicConfigClientUpdater>()
                              .Append<components::HttpClient>()
                              .Append<clients::dns::Component>()
                              .Append<chaotic::openapi::QosMiddlewareFactory<clients::test::kQosConfig>>(
                                  "chaotic-client-middleware-qos-test"
                              )
                              .Append<clients::test::Component>();

    chaotic::openapi::middlewares::AppendDefaultMiddlewares(component_list);

    return utils::DaemonMain(argc, argv, component_list);
}
