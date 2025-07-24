#include <userver/utest/using_namespace_userver.hpp>

#include <fmt/format.h>

#include <userver/components/component.hpp>
#include <userver/components/component_base.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/congestion_control/component.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/utils/daemon_run.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <userver/ugrpc/server/component_list.hpp>
#include <userver/ugrpc/server/health/component.hpp>
#include <userver/ugrpc/server/service_component_base.hpp>

int main(int argc, char* argv[]) {
    const auto component_list = components::MinimalServerComponentList()
                                    .AppendComponentList(ugrpc::server::MinimalComponentList())
                                    .Append<congestion_control::Component>()
                                    .Append<ugrpc::server::HealthComponent>();
    return utils::DaemonMain(argc, argv, component_list);
}
