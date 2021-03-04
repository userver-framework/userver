#include <components/common_component_list.hpp>

#include <components/minimal_component_list.hpp>

#include <clients/http/component.hpp>
#include <components/logging_configurator.hpp>
#include <taxi_config/configs/component.hpp>
#include <taxi_config/updater/client/component.hpp>
#include <testsuite/testsuite_support.hpp>
#include <utils/statistics/system_statistics_collector.hpp>

namespace components {

ComponentList CommonComponentList() {
  return components::MinimalComponentList()
      .Append<components::LoggingConfigurator>()
      .Append<components::TestsuiteSupport>()
      .Append<components::SystemStatisticsCollector>()
      .Append<components::HttpClient>()
      .Append<components::HttpClient>("http-client-statistics")
      .Append<components::TaxiConfigClient>()
      .Append<components::TaxiConfigClientUpdater>();
}

}  // namespace components
