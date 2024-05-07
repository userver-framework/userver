#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/components/component.hpp>
#include <userver/components/loggable_component_base.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/dynamic_config/client/component.hpp>
#include <userver/dynamic_config/storage/component.hpp>
#include <userver/dynamic_config/updater/component.hpp>
#include <userver/server/component.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/storages/secdist/provider_component.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/daemon_run.hpp>

#include <userver/ydb/component.hpp>

#include <views/bson-reading/post/view.hpp>
#include <views/bson-upserting/post/view.hpp>
#include <views/select-rows/post/view.hpp>
#include <views/upsert-2rows/post/view.hpp>
#include <views/upsert-row/post/view.hpp>
#include <views/upsert-rows/post/view.hpp>

#include <components/topic_reader.hpp>

int main(int argc, char* argv[]) {
  auto component_list = ::components::MinimalServerComponentList()
                            .Append<components::TestsuiteSupport>()
                            .Append<server::handlers::TestsControl>()
                            .Append<components::DynamicConfigClient>()
                            .Append<components::DynamicConfigClientUpdater>()
                            .Append<components::HttpClient>()
                            .Append<clients::dns::Component>()
                            .Append<components::DefaultSecdistProvider>()
                            .Append<components::Secdist>()
                            .Append<sample::BsonReadingHandler>()
                            .Append<sample::BsonUpsertingHandler>()
                            .Append<sample::SelectRowsHandler>()
                            .Append<sample::Upsert2RowsHandler>()
                            .Append<sample::UpsertRowHandler>()
                            .Append<sample::UpsertRowsHandler>()
                            .Append<sample::TopicReaderComponent>()
                            .Append<ydb::YdbComponent>();

  return ::utils::DaemonMain(argc, argv, component_list);
}
