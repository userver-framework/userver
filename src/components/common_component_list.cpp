#include <components/common_component_list.hpp>

#include <components/cache_invalidator.hpp>
#include <components/manager_controller_component.hpp>
#include <components/statistics_storage.hpp>
#include <components/tracer.hpp>
#include <logging/component.hpp>
#include <storages/mongo/component.hpp>
#include <storages/secdist/component.hpp>
#include <taxi_config/storage/component.hpp>
#include <taxi_config/updater/mongo/component.hpp>

namespace components {

ComponentList CommonComponentList() {
  return components::ComponentList()
      .Append<components::Logging>()
      .Append<components::Tracer>()
      .Append<components::ManagerControllerComponent>()
      .Append<components::Secdist>()
      .Append<components::CacheInvalidator>()
      .Append<components::StatisticsStorage>()
      .Append<components::Mongo>("mongo-taxi")
      .Append<components::TaxiConfig>()
      .Append<components::TaxiConfigMongoUpdater>();
}

}  // namespace components
