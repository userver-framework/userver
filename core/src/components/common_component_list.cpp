#include <userver/components/common_component_list.hpp>

#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/components/dump_configurator.hpp>
#include <userver/components/headers_propagator_component.hpp>
#include <userver/components/logging_configurator.hpp>
#include <userver/components/manager_controller_component.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/components/tracer.hpp>
#include <userver/dynamic_config/client/component.hpp>
#include <userver/dynamic_config/storage/component.hpp>
#include <userver/dynamic_config/updater/component.hpp>
#include <userver/logging/component.hpp>
#include <userver/os_signals/component.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/statistics/system_statistics_collector.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

ComponentList CommonComponentList() {
  return components::ComponentList()
      .Append<os_signals::ProcessorComponent>()
      .Append<components::Logging>()
      .Append<components::Tracer>()
      .Append<components::ManagerControllerComponent>()
      .Append<components::StatisticsStorage>()
      .Append<components::DynamicConfig>()

      .Append<components::LoggingConfigurator>()
      .Append<components::DumpConfigurator>()
      .Append<components::TestsuiteSupport>()
      .Append<components::SystemStatisticsCollector>()
      .Append<components::HttpClient>()
      .Append<components::HttpClient>("http-client-statistics")
      .Append<clients::dns::Component>()
      .Append<components::DynamicConfigClient>()
      .Append<components::DynamicConfigClientUpdater>();
}

}  // namespace components

USERVER_NAMESPACE_END
