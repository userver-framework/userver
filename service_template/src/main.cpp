#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/ping.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/storages/mongo/component.hpp>     // mongo template current
#include <userver/storages/postgres/component.hpp>  // postgres template current
#include <userver/testsuite/testsuite_support.hpp>

#include <userver/utils/daemon_run.hpp>

#include "hello.hpp"

int main(int argc, char* argv[]) {
    auto component_list = userver::components::MinimalServerComponentList()
                              .Append<userver::server::handlers::Ping>()
                              .Append<userver::components::TestsuiteSupport>()
                              .Append<userver::components::HttpClient>()
                              // postgres template on
                              .Append<userver::components::Postgres>("postgres-db-1")
                              // postgres template off
                              // mongo template on
                              .Append<userver::components::Mongo>("mongo-db-1")
                              // mongo template off
                              .Append<userver::clients::dns::Component>()
                              .Append<userver::server::handlers::TestsControl>();

    service_template::AppendHello(component_list);

    return userver::utils::DaemonMain(argc, argv, component_list);
}
