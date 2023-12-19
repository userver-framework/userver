#include <userver/components/common_component_list.hpp>

#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/components/dump_configurator.hpp>
#include <userver/components/logging_configurator.hpp>
#include <userver/components/minimal_component_list.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/dynamic_config/client/component.hpp>
#include <userver/dynamic_config/updater/component.hpp>
#include <userver/engine/task_processors_load_monitor.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/tracing/manager_component.hpp>
#include <userver/utils/statistics/system_statistics_collector.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

ComponentList CommonComponentList() {
  return components::ComponentList()
      .AppendComponentList(components::MinimalComponentList())

      .Append<components::LoggingConfigurator>()
      .Append<components::DumpConfigurator>()
      .Append<components::TestsuiteSupport>()
      .Append<components::SystemStatisticsCollector>()
      .Append<components::HttpClient>()
      .Append<components::HttpClient>("http-client-statistics")
      .Append<clients::dns::Component>()
      .Append<components::DynamicConfigClient>()
      .Append<components::DynamicConfigClientUpdater>()

      .Append<engine::TaskProcessorsLoadMonitor>();
}

}  // namespace components

USERVER_NAMESPACE_END
