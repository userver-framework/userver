#include <components/minimal_component_list.hpp>

#include <components/manager_controller_component.hpp>
#include <components/statistics_storage.hpp>
#include <components/tracer.hpp>
#include <logging/component.hpp>
#include <taxi_config/storage/component.hpp>

namespace components {

ComponentList MinimalComponentList() {
  return ::components::ComponentList()
      .Append<components::Logging>()
      .Append<components::Tracer>()
      .Append<components::ManagerControllerComponent>()
      .Append<components::StatisticsStorage>()
      .Append<components::TaxiConfig>();
}

}  // namespace components
