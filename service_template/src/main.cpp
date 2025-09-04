#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/components/component.hpp>
#include <userver/components/component_list.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/congestion_control/component.hpp>
#include <userver/server/handlers/ping.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/testsuite/testsuite_support.hpp>

#include <userver/storages/mongo/component.hpp>     // mongo template current
#include <userver/storages/postgres/component.hpp>  // postgresql template current
#include <userver/ugrpc/server/component_list.hpp>  // grpc template current

#include <userver/utils/daemon_run.hpp>

#include <hello.hpp>
#include <hello_grpc.hpp>      // grpc template current
#include <hello_mongo.hpp>     // mongo template current
#include <hello_postgres.hpp>  // postgresql template current

int main(int argc, char* argv[]) {
    auto component_list = userver::components::MinimalServerComponentList()
                              .Append<userver::server::handlers::Ping>()
                              .Append<userver::components::TestsuiteSupport>()
                              .Append<userver::components::HttpClient>()
                              .Append<userver::clients::dns::Component>()
                              .Append<userver::server::handlers::TestsControl>()
                              .Append<userver::congestion_control::Component>()
                              .Append<service_template::Hello>()
                              // postgresql template on
                              .Append<userver::components::Postgres>("postgres-db-1")
                              .Append<service_template::HelloPostgres>()
                              // postgresql template off
                              // mongo template on
                              .Append<userver::components::Mongo>("mongo-db-1")
                              .Append<service_template::HelloMongo>()
                              // mongo template off
                              // grpc template on
                              .AppendComponentList(userver::ugrpc::server::MinimalComponentList())
                              .Append<service_template::HelloGrpc>()
        // grpc template off
        ;

    return userver::utils::DaemonMain(argc, argv, component_list);
}
