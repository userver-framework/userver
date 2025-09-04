#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/components/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utest/using_namespace_userver.hpp>
#include <userver/utils/daemon_run.hpp>

#include <userver/storages/sqlite/component.hpp>

#include <handlers/batch.hpp>
#include <handlers/key_value.hpp>

int main(int argc, char* argv[]) {
    auto component_list = components::MinimalServerComponentList()
                              .Append<components::SQLite>("key-value-database")
                              .Append<components::SQLite>("batch-database")
                              .Append<components::HttpClient>()
                              .Append<server::handlers::TestsControl>()
                              .Append<components::TestsuiteSupport>()
                              .Append<clients::dns::Component>();
    functional_tests::AppendKeyValue(component_list);
    functional_tests::AppendBatch(component_list);
    return utils::DaemonMain(argc, argv, component_list);
}
