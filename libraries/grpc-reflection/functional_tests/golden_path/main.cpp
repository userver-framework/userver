#include <userver/utest/using_namespace_userver.hpp>  // IWYU pragma: keep

#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/components/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/congestion_control/component.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/storages/secdist/provider_component.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/ugrpc/server/component_list.hpp>
#include <userver/ugrpc/server/health/component.hpp>
#include <userver/utils/daemon_run.hpp>

#include <userver/grpc-reflection/reflection_service_component.hpp>

int main(int argc, char* argv[]) {
    const auto component_list = components::MinimalServerComponentList()
                                    .Append<congestion_control::Component>()
                                    .Append<grpc_reflection::ReflectionServiceComponent>()
                                    .Append<components::TestsuiteSupport>()
                                    .Append<server::handlers::TestsControl>()
                                    .Append<ugrpc::server::HealthComponent>()
                                    .Append<components::HttpClient>()
                                    .AppendComponentList(ugrpc::server::DefaultComponentList())
                                    .Append<clients::dns::Component>();
    return utils::DaemonMain(argc, argv, component_list);
}
