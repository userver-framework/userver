#pragma once

/// @file userver/dynamic_config/updater/component.hpp
/// @brief @copybrief components::DynamicConfigClientUpdater

#include <chrono>
#include <functional>
#include <optional>
#include <string>
#include <unordered_set>

#include <userver/cache/cache_statistics.hpp>
#include <userver/cache/caching_component_base.hpp>
#include <userver/cache/update_type.hpp>
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
/// ## Optional update event deduplication
///
/// Config update types to deduplicate. If enabled, JSON of the whole config is
/// compared to the previous one; if same, no config update event is sent to the
/// subscribers of dynamic_config::Source (`OnConfigUpdate` functions).
///
/// `deduplicate-update-types` static config option specifies the update types
/// of the config cache, for which event deduplication should be performed.
/// Possible values:
/// - `none` (the default)
/// - `only-full`
/// - `only-incremental`
/// - `full-and-incremental`
///
/// Full updates will always send an event unless deduplicated. Incremental
/// updates may send an extra event for some config service implementations.
///
/// Note: This is not a silver bullet against extra events, because the events
/// will be sent to every dynamic config subscriber if *any* part of the config
/// has updated, not if the interesting part has updated.
///
/// ## Static options:
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// store-enabled | store the retrieved values into the components::dynamicConfig | -
/// load-only-my-values | request from the client only the values used by this service | -
/// fallback-path | a path to the fallback config to load the required config names from it | -
/// fs-task-processor | name of the task processor to run the blocking file write operations | -
/// deduplicate-update-types | update types for best-effort update event deduplication, see above | `full-and-incremental`
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
  static constexpr std::string_view kName = "dynamic-config-client-updater";

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

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  void StoreIfEnabled();

  using DocsMapKeys = std::unordered_set<std::string>;
  using AdditionalDocsMapKeys =
      std::unordered_set<std::shared_ptr<std::vector<std::string>>>;

  std::vector<std::string> GetDocsMapKeysToFetch(
      AdditionalDocsMapKeys& additional_docs_map_keys);

  void UpdateAdditionalKeys(const std::vector<std::string>& keys);

  bool IsDuplicate(cache::UpdateType update_type,
                   const dynamic_config::DocsMap& new_value) const;

  dynamic_config::DocsMap fallback_config_;
  dynamic_config::Client::Timestamp server_timestamp_;

  components::DynamicConfig::Updater<DynamicConfigClientUpdater> updater_;

  const bool load_only_my_values_;
  const bool store_enabled_;
  const std::optional<cache::AllowedUpdateTypes> deduplicate_update_types_;

  dynamic_config::Client& config_client_;

  // for atomic updates of cached data
  engine::Mutex update_config_mutex_;

  DocsMapKeys docs_map_keys_;
  concurrent::Variable<AdditionalDocsMapKeys> additional_docs_map_keys_;
};

template <>
inline constexpr bool kHasValidate<DynamicConfigClientUpdater> = true;

}  // namespace components

USERVER_NAMESPACE_END
