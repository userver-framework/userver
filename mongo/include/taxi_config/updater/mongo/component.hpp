#pragma once

#include <cache/cache_statistics.hpp>
#include <cache/caching_component_base.hpp>
#include <chrono>
#include <components/component_config.hpp>
#include <components/component_context.hpp>
#include <functional>
#include <storages/mongo/pool.hpp>
#include <string>
#include <taxi_config/storage/component.hpp>
#include <taxi_config/value.hpp>

namespace components {

class TaxiConfigMongoUpdater
    : public CachingComponentBase<taxi_config::DocsMap> {
 public:
  static constexpr auto kName = "taxi-config-mongo-updater";

  using EmplaceDocsCb = std::function<void(taxi_config::DocsMap&& mongo_docs)>;

  TaxiConfigMongoUpdater(const ComponentConfig&, const ComponentContext&);

  ~TaxiConfigMongoUpdater();

  void Update(cache::UpdateType type,
              const std::chrono::system_clock::time_point& last_update,
              const std::chrono::system_clock::time_point& now,
              cache::UpdateStatisticsScope&) override;

 private:
  std::chrono::system_clock::time_point seen_doc_update_time_;
  storages::mongo::PoolPtr mongo_taxi_;
  taxi_config::DocsMap fallback_config_;
  components::TaxiConfig& taxi_config_;
};

}  // namespace components
