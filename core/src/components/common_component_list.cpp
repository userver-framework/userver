#include <userver/components/common_component_list.hpp>

#include <userver/components/minimal_component_list.hpp>

#include <userver/clients/http/component.hpp>
#include <userver/components/dump_configurator.hpp>
#include <userver/components/logging_configurator.hpp>
#include <userver/taxi_config/configs/component.hpp>
#include <userver/taxi_config/updater/client/component.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/statistics/system_statistics_collector.hpp>

namespace components {

ComponentList CommonComponentList() {
  return components::MinimalComponentList()
      .Append<components::LoggingConfigurator>()
      .Append<components::DumpConfigurator>()
      .Append<components::TestsuiteSupport>()
      .Append<components::SystemStatisticsCollector>()
      .Append<components::HttpClient>()
      .Append<components::HttpClient>("http-client-statistics")
      .Append<components::TaxiConfigClient>()
      .Append<components::TaxiConfigClientUpdater>();
}

}  // namespace components
