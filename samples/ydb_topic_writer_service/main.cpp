#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component_list.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/dynamic_config/updater/component_list.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/storages/secdist/provider_component.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/daemon_run.hpp>

#include <userver/ydb/component.hpp>
#include <userver/ydb/topic_writer_component.hpp>

#include <write_handler.hpp>

/// [YDB topic writer service sample - main]
int main(int argc, char* argv[]) {
    const auto component_list =
        components::MinimalServerComponentList()
            .AppendComponentList(USERVER_NAMESPACE::dynamic_config::updater::ComponentList())
            .Append<components::TestsuiteSupport>()
            .Append<server::handlers::TestsControl>()
            .AppendComponentList(clients::http::ComponentList())
            .Append<clients::dns::Component>()
            .Append<components::DefaultSecdistProvider>()
            .Append<components::Secdist>()
            .Append<samples::ydb_topic_writer::WriteHandler>()
            .Append<ydb::YdbComponent>()
            .Append<ydb::TopicWriterComponent>();

    return utils::DaemonMain(argc, argv, component_list);
}
/// [YDB topic writer service sample - main]
