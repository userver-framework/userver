#include <userver/components/minimal_component_list.hpp>

#include <userver/components/manager_controller_component.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/components/tracer.hpp>
#include <userver/logging/component.hpp>
#include <userver/taxi_config/storage/component.hpp>

namespace components {

ComponentList MinimalComponentList() {
  return ::components::ComponentList()
      .Append<components::Logging>()
      .Append<components::Tracer>()
      .Append<components::ManagerControllerComponent>()
      .Append<components::StatisticsStorage>()
      .Append<components::TaxiConfig>()
      .Append<components::TaxiConfigFallbacksComponent>();
}

}  // namespace components
