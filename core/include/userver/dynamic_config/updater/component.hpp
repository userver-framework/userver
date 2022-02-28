#pragma once

/// @file userver/dynamic_config/updater/component.hpp
/// @brief @copybrief components::DynamicConfigClientUpdater

#include <chrono>
#include <functional>
#include <string>
#include <unordered_set>

#include <userver/cache/cache_statistics.hpp>
#include <userver/cache/caching_component_base.hpp>
#include <userver/components/component_fwd.hpp>
#include <userver/concurrent/variable.hpp>
#include <userver/dynamic_config/client/client.hpp>
#include <userver/dynamic_config/snapshot.hpp>
#include <userver/dynamic_config/storage/component.hpp>
#include <userver/dynamic_config/updater/additional_keys_token.hpp>
#include <userver/engine/mutex.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

// clang-format off

/// @ingroup userver_components
///
/// @brief Component that does a periodic update of runtime configs.
///
/// ## Static options:
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// store-enabled | store the retrived values into the components::dynamicConfig | -
/// load-only-my-values | request from the client only the values used by this service | -
/// fallback-path | a path to the fallback config to load the required config names from it | -
///
/// See also the options for components::CachingComponentBase.
///
/// ## Static configuration example:
///
/// @snippet components/common_component_list_test.cpp  Sample dynamic config client updater component config

// clang-format on
class DynamicConfigClientUpdater
    : public CachingComponentBase<dynamic_config::DocsMap> {
 public:
  static constexpr auto kName = "taxi-config-client-updater";

  DynamicConfigClientUpdater(const ComponentConfig&, const ComponentContext&);

  ~DynamicConfigClientUpdater() override;

  // After calling this method, `Get()` will return a dynamic_config containing
  // the specified keys while the token that this method returned is alive.
  dynamic_config::AdditionalKeysToken SetAdditionalKeys(
      std::vector<std::string> keys);

  void Update(cache::UpdateType update_type,
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
  dynamic_config::DocsMap fallback_config_;
  dynamic_config::Client::Timestamp server_timestamp_;

  components::DynamicConfig::Updater<DynamicConfigClientUpdater> updater_;

  const bool load_only_my_values_;
  const bool store_enabled_;

  dynamic_config::Client& config_client_;

  // for atomic updates of cached data
  engine::Mutex update_config_mutex_;

  concurrent::Variable<DocsMapKeys> docs_map_keys_;
  concurrent::Variable<AdditionalDocsMapKeys> additional_docs_map_keys_;
};

using TaxiConfigClientUpdater = DynamicConfigClientUpdater;

}  // namespace components

USERVER_NAMESPACE_END
