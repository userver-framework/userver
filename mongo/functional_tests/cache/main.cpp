#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component_list.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/dynamic_config/updater/component_list.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/storages/mongo/component.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/daemon_run.hpp>

#include <userver/utest/using_namespace_userver.hpp>

#include <cache_state_handler.hpp>
#include <default_query_cache.hpp>
#include <mongo_collections.hpp>
#include <runtime_query_cache.hpp>

int main(int argc, char* argv[]) {
    const auto component_list =
        components::MinimalServerComponentList()
            .AppendComponentList(USERVER_NAMESPACE::dynamic_config::updater::ComponentList())
            .Append<clients::dns::Component>()
            .AppendComponentList(clients::http::ComponentList())
            .Append<components::TestsuiteSupport>()
            .Append<server::handlers::TestsControl>()
            .Append<components::Mongo>("cache-database")
            .Append<functional_tests::MongoCollections>()
            .Append<functional_tests::RuntimeQueryCache>()
            .Append<functional_tests::DefaultQueryCache>()
            .Append<functional_tests::CacheStateHandler>();
    return utils::DaemonMain(argc, argv, component_list);
}
