#pragma once

/// @file userver/taxi_config/updater/client/component.hpp
/// @brief @copybrief components::TaxiConfigClientUpdater

#include <chrono>
#include <functional>
#include <string>
#include <unordered_set>

#include <userver/cache/cache_statistics.hpp>
#include <userver/cache/caching_component_base.hpp>
#include <userver/clients/config/client.hpp>
#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/concurrent/variable.hpp>
#include <userver/engine/mutex.hpp>
#include <userver/taxi_config/additional_keys_token.hpp>
#include <userver/taxi_config/snapshot.hpp>
#include <userver/taxi_config/storage/component.hpp>

namespace components {
// clang-format off

/// @ingroup userver_components
///
/// @brief Component that does a periodic update of runtime configs.
///
/// ## Static options:
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// store-enabled | store the retrived values into the components::TaxiConfig | -
/// load-only-my-values | request from the client only the values used by this service | -
/// fallback-path | a path to the fallback config to load the required config names from it | -
///
/// See also the options for components::CachingComponentBase.
///
/// ## Static configuration example:
///
/// @snippet components/common_component_list_test.cpp  Sample taxi config client updater component config

// clang-format on
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

  components::TaxiConfig::Updater taxi_config_updater_;
  const bool load_only_my_values_;
  const bool store_enabled_;

  clients::taxi_config::Client& config_client_;

  // for atomic updates of cached data
  engine::Mutex update_config_mutex_;

  concurrent::Variable<DocsMapKeys> docs_map_keys_;
  concurrent::Variable<AdditionalDocsMapKeys> additional_docs_map_keys_;
};

}  // namespace components
