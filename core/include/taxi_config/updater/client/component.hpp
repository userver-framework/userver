#pragma once

#include <chrono>
#include <functional>
#include <string>
#include <unordered_set>

#include <cache/cache_statistics.hpp>
#include <cache/caching_component_base.hpp>
#include <clients/config/client.hpp>
#include <components/component_config.hpp>
#include <components/component_context.hpp>
#include <concurrent/variable.hpp>
#include <engine/mutex.hpp>
#include <taxi_config/additional_keys_token.hpp>
#include <taxi_config/config.hpp>
#include <taxi_config/storage/component.hpp>

namespace components {

class TaxiConfigClientUpdater
    : public CachingComponentBase<taxi_config::DocsMap> {
 public:
  static constexpr auto kName = "taxi-config-client-updater";

  TaxiConfigClientUpdater(const ComponentConfig&, const ComponentContext&);

  ~TaxiConfigClientUpdater() override;

  // After calling this method, `Get()` will return a taxi_config containing the
  // specified keys while the token that this method returned is alive.
  taxi_config::AdditionalKeysToken SetAdditionalKeys(
      std::vector<std::string> keys);

  void Update(cache::UpdateType type,
              const std::chrono::system_clock::time_point& last_update,
              const std::chrono::system_clock::time_point& now,
              cache::UpdateStatisticsScope&) override;

 private:
  void StoreIfEnabled();

  using DocsMapKeys = std::unordered_set<std::string>;
  using AdditionalDocsMapKeys =
      std::unordered_set<std::shared_ptr<std::vector<std::string>>>;

  DocsMapKeys GetStoredDocsMapKeys() const;

  std::vector<std::string> GetDocsMapKeysToFetch(
      AdditionalDocsMapKeys& additional_docs_map_keys);

  void UpdateAdditionalKeys(const std::vector<std::string>& keys);

 private:
  ::taxi_config::DocsMap fallback_config_;
  clients::taxi_config::Client::Timestamp server_timestamp_;

  components::TaxiConfig& taxi_config_;
  const bool load_only_my_values_;
  const bool store_enabled_;

  clients::taxi_config::Client& config_client_;

  // for atomic updates of cached data
  engine::Mutex update_config_mutex_;

  concurrent::Variable<DocsMapKeys> docs_map_keys_;
  concurrent::Variable<AdditionalDocsMapKeys> additional_docs_map_keys_;
};

}  // namespace components
