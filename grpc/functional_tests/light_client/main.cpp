#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component_list.hpp>
#include <userver/components/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/congestion_control/component.hpp>
#include <userver/server/handlers/server_monitor.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utest/using_namespace_userver.hpp>
#include <userver/utils/daemon_run.hpp>

#include <userver/ugrpc/client/client_factory_component.hpp>
#include <userver/ugrpc/client/component_list.hpp>
#include <userver/ugrpc/server/component_list.hpp>

#include <blocking_retry_limiter.hpp>
#include <config_pusher.hpp>
#include <light_client_test_component.hpp>
#include <non_blocking_retry_limiter.hpp>

int main(int argc, const char* const argv[]) {
    const auto component_list =
        components::MinimalServerComponentList()
            .Append<server::handlers::ServerMonitor>()
            .Append<congestion_control::Component>()
            .Append<components::TestsuiteSupport>()
            .Append<server::handlers::TestsControl>()
            .Append<clients::dns::Component>()
            .AppendComponentList(clients::http::ComponentList())
            .AppendComponentList(ugrpc::client::DefaultComponentList())
            // No registered gRPC services; see static_config.yaml's
            // grpc-server entry for why it's still present.
            .AppendComponentList(ugrpc::server::MinimalComponentList())
            .Append<functional_tests::BlockingRetryLimiterComponent>()
            .Append<functional_tests::NonBlockingRetryLimiterComponent>()
            // See static_config.yaml for what each factory below proves.
            .Append<ugrpc::client::ClientFactoryComponent>()
            .Append<ugrpc::client::ClientFactoryComponent>("light-client-factory")
            .Append<ugrpc::client::ClientFactoryComponent>("light-client-factory-explicit-retry-limiter")
            .Append<ugrpc::client::ClientFactoryComponent>("regular-client-factory-no-retry-limiter")
            .Append<functional_tests::ClientComponent>("regular-greeter-client")
            .Append<functional_tests::ClientComponent>("light-greeter-client")
            .Append<functional_tests::ClientComponent>("light-greeter-client-explicit-retry-limiter")
            .Append<functional_tests::ClientComponent>("regular-greeter-client-no-retry-limiter")
            .Append<functional_tests::LightClientTestComponent>()
            // This service's only source of dynamic config updates; see
            // config_pusher.hpp.
            .Append<functional_tests::ConfigPusher>();

    return utils::DaemonMain(argc, argv, component_list);
}
