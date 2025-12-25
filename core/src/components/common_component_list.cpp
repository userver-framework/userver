#include <userver/components/common_component_list.hpp>

#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/clients/http/component_list.hpp>
#include <userver/components/dump_configurator.hpp>
#include <userver/components/logging_configurator.hpp>
#include <userver/components/minimal_component_list.hpp>
#include <userver/dynamic_config/updater/component_list.hpp>
#include <userver/engine/task_processors_load_monitor.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/statistics/system_statistics_collector.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

ComponentList CommonComponentList() {
    return components::ComponentList()
        .AppendComponentList(components::MinimalComponentList())
        .AppendComponentList(clients::http::ComponentList())
        .AppendComponentList(USERVER_NAMESPACE::dynamic_config::updater::ComponentList())

        .Append<components::LoggingConfigurator>()
        .Append<components::DumpConfigurator>()
        .Append<components::TestsuiteSupport>()
        .Append<components::SystemStatisticsCollector>()
        .Append<components::HttpClientCore>("http-client-statistics-core")
        .Append<components::HttpClient>("http-client-statistics")
        .Append<clients::dns::Component>()

        .Append<engine::TaskProcessorsLoadMonitor>();
}

}  // namespace components

USERVER_NAMESPACE_END
