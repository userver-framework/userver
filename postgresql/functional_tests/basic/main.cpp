#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component_list.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/daemon_run.hpp>

#include <userver/utest/using_namespace_userver.hpp>

#include <insert_value.hpp>
#include <select_value.hpp>

int main(int argc, char* argv[]) {
    const auto component_list =
        components::MinimalServerComponentList()
            .Append<postgresql_tests_basic::InsertValue>()
            .Append<postgresql_tests_basic::SelectValue>()
            .Append<components::Postgres>("key-value-database")
            .AppendComponentList(clients::http::ComponentList())
            .Append<components::TestsuiteSupport>()
            .Append<server::handlers::TestsControl>()
            .Append<clients::dns::Component>();
    return utils::DaemonMain(argc, argv, component_list);
}
