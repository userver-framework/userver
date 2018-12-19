#pragma once

#include <chrono>
#include <functional>
#include <string>

#include <cache/cache_statistics.hpp>
#include <clients/config/client.hpp>
#include <components/caching_component_base.hpp>
#include <components/component_config.hpp>
#include <components/component_context.hpp>
#include <taxi_config/config.hpp>
#include <taxi_config/storage/component.hpp>

namespace components {

class TaxiConfigClientUpdater
    : public CachingComponentBase<taxi_config::DocsMap> {
 public:
  static constexpr auto kName = "taxi-config-client-updater";

  TaxiConfigClientUpdater(const ComponentConfig&, const ComponentContext&);

  ~TaxiConfigClientUpdater();

  void Update(cache::UpdateType type,
              const std::chrono::system_clock::time_point& last_update,
              const std::chrono::system_clock::time_point& now,
              cache::UpdateStatisticsScope&) override;

 private:
  void StoreIfEnabled();

  std::vector<std::string> GetDocsMapKeys() const;

 private:
  ::taxi_config::DocsMap fallback_config_;
  clients::taxi_config::Client::Timestamp server_timestamp_;
  components::TaxiConfig& taxi_config_;
  std::unique_ptr<clients::taxi_config::Client> config_client_;
  bool store_enabled_;

  bool load_only_my_values_;
  std::vector<std::string> docs_map_keys_;
};

}  // namespace components
