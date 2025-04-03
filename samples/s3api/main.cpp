#include <userver/components/minimal_server_component_list.hpp>
#include <userver/utest/using_namespace_userver.hpp>
#include <userver/utils/daemon_run.hpp>

#include <s3api_client.hpp>

/// [main]
int main(int argc, char* argv[]) {
    const auto component_list = components::MinimalServerComponentList().Append<samples::S3ApiSampleComponent>();

    return utils::DaemonMain(argc, argv, component_list);
}
/// [main]
