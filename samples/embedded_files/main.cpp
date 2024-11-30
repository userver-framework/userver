#include <userver/components/minimal_server_component_list.hpp>
#include <userver/utest/using_namespace_userver.hpp>
#include <userver/utils/daemon_run.hpp>

// Note: this is for the purposes of tests/samples only
#include <userver/utest/using_namespace_userver.hpp>

#include <hello_handler.hpp>

#include <generated/static_config.yaml.hpp>

// [embedded usage]
int main(int, char*[]) {
    auto component_list = components::MinimalServerComponentList().Append<samples::hello::HelloHandler>();

    return utils::DaemonMain(components::InMemoryConfig{utils::FindResource("static_config_yaml")}, component_list);
}
// [embedded usage]
