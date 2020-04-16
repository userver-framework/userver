#include <components/common_component_list.hpp>

#include <clients/http/component.hpp>
#include <components/manager_controller_component.hpp>
#include <components/statistics_storage.hpp>
#include <components/tracer.hpp>
#include <congestion_control/component.hpp>
#include <logging/component.hpp>
#include <taxi_config/configs/component.hpp>
#include <taxi_config/storage/component.hpp>
#include <taxi_config/updater/client/component.hpp>
#include <testsuite/testsuite_support.hpp>

namespace components {

ComponentList CommonComponentList() {
  return components::ComponentList()
      .Append<components::Logging>()
      .Append<components::Tracer>()
      .Append<components::ManagerControllerComponent>()
      .Append<components::TestsuiteSupport>()
      .Append<components::StatisticsStorage>()
      .Append<components::TaxiConfig>()
      .Append<congestion_control::Component>()
      .Append<components::HttpClient>()
      .Append<components::HttpClient>("http-client-statistics")
      .Append<components::TaxiConfigClient>()
      .Append<components::TaxiConfigClientUpdater>();
}

}  // namespace components
