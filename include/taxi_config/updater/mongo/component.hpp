#pragma once

#include <chrono>
#include <functional>
#include <string>

#include <cache/cache_statistics.hpp>
#include <components/caching_component_base.hpp>
#include <components/component_config.hpp>
#include <components/component_context.hpp>
#include <storages/mongo/pool.hpp>
#include <taxi_config/config.hpp>
#include <taxi_config/storage/component.hpp>

namespace components {

class TaxiConfigMongoUpdater
    : public CachingComponentBase<taxi_config::Config> {
 public:
  static constexpr auto kName = "taxi-config-mongo-updater";

  using EmplaceDocsCb = std::function<void(taxi_config::DocsMap&& mongo_docs)>;

  TaxiConfigMongoUpdater(const ComponentConfig&, const ComponentContext&);

  ~TaxiConfigMongoUpdater();

  void Update(CacheUpdateTrait::UpdateType type,
              const std::chrono::system_clock::time_point& last_update,
              const std::chrono::system_clock::time_point& now,
              tracing::Span&& span);

 private:
  utils::SwappingSmart<taxi_config::Config> cache_;
  std::chrono::system_clock::time_point seen_doc_update_time_;
  storages::mongo::PoolPtr mongo_taxi_;
  taxi_config::DocsMap fallback_config_;
  components::TaxiConfig& taxi_config_;
};

}  // namespace components
