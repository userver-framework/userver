#include <userver/components/minimal_server_component_list.hpp>
#include <userver/congestion_control/component.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/ugrpc/server/component_list.hpp>
#include <userver/utils/daemon_run.hpp>

#include <error_status_server.hpp>

int main(int argc, const char* const argv[]) {
    const auto component_list =
        components::MinimalServerComponentList()
            .Append<components::TestsuiteSupport>()
            .Append<congestion_control::Component>()
            .AppendComponentList(ugrpc::server::MinimalComponentList())
            .Append<functional_tests::ErrorStatusServiceComponent>();

    return utils::DaemonMain(argc, argv, component_list);
}
