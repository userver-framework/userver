#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component_list.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utest/using_namespace_userver.hpp>
#include <userver/utils/daemon_run.hpp>

#include <s3api_client.hpp>

/// [main]
int main(int argc, char* argv[]) {
    const auto component_list =
        components::MinimalServerComponentList()
            .Append<clients::dns::Component>()
            .Append<components::TestsuiteSupport>()
            .Append<server::handlers::TestsControl>()
            .AppendComponentList(clients::http::ComponentList())
            .Append<samples::S3ApiSampleComponent>();

    return utils::DaemonMain(argc, argv, component_list);
}
/// [main]
