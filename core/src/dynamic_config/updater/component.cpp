#include <userver/dynamic_config/updater/component.hpp>

#include <userver/cache/update_type.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/components/component.hpp>
#include <userver/dynamic_config/client/component.hpp>
#include <userver/dynamic_config/updates_sink/find.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/fs/read.hpp>
#include <userver/utils/string_to_duration.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

namespace {

std::optional<cache::AllowedUpdateTypes> ParseDeduplicateUpdateTypes(
    const yaml_config::YamlConfig& value) {
  const auto str = value.As<std::optional<std::string>>();
  if (!str) return cache::AllowedUpdateTypes::kFullAndIncremental;
  if (str == "none") return std::nullopt;
  return value.As<cache::AllowedUpdateTypes>();
}

bool ShouldDeduplicate(
    const std::optional<cache::AllowedUpdateTypes>& update_types,
    cache::UpdateType current_update_type) {
  switch (current_update_type) {
    case cache::UpdateType::kFull:
      return update_types == cache::AllowedUpdateTypes::kOnlyFull ||
             update_types == cache::AllowedUpdateTypes::kFullAndIncremental;
    case cache::UpdateType::kIncremental:
      return update_types == cache::AllowedUpdateTypes::kOnlyIncremental ||
             update_types == cache::AllowedUpdateTypes::kFullAndIncremental;
  }
  UINVARIANT(false, "Invalid cache::UpdateType");
}

void CheckUnusedConfigs(
    const dynamic_config::DocsMap& configs,
    const std::unordered_set<std::string>& statically_required_configs) {
  const auto& used_configs = configs.GetRequestedNames();
  std::vector<std::string_view> extra_configs;
  for (const auto& statically_required_config : statically_required_configs) {
    if (used_configs.find(statically_required_config) == used_configs.end()) {
      extra_configs.push_back(statically_required_config);
    }
  }
  if (!extra_configs.empty()) {
    LOG_INFO() << "Some statically required configs are unused: "
               << extra_configs;
  }
}

}  // namespace

DynamicConfigClientUpdater::DynamicConfigClientUpdater(
    const ComponentConfig& component_config,
    const ComponentContext& component_context)
    : CachingComponentBase(component_config, component_context),
      updates_sink_(
          dynamic_config::FindUpdatesSink(component_config, component_context)),
      load_only_my_values_(component_config["load-only-my-values"].As<bool>()),
      store_enabled_(component_config["store-enabled"].As<bool>()),
      deduplicate_update_types_(ParseDeduplicateUpdateTypes(
          component_config["deduplicate-update-types"])),
      config_client_(
          component_context.FindComponent<components::DynamicConfigClient>()
              .GetClient()) {
  auto tp_name =
      component_config["fs-task-processor"].As<std::optional<std::string>>();
  auto& tp = tp_name ? component_context.GetTaskProcessor(*tp_name)
                     : engine::current_task::GetTaskProcessor();

  auto fallback_config_contents = fs::ReadFileContents(
      tp, component_config["fallback-path"].As<std::string>());

  try {
    fallback_config_.Parse(fallback_config_contents, false);

    // There are all required configs in the fallbacks file
    docs_map_keys_ = fallback_config_.GetNames();
  } catch (const std::exception& ex) {
    throw std::runtime_error(
        std::string("Cannot load fallback dynamic config: ") + ex.what());
  }

  try {
    StartPeriodicUpdates();
  } catch (const std::exception& e) {
    LOG_ERROR() << "Config client updater initialization failed: " << e;
    updates_sink_.NotifyLoadingFailed(kName, e.what());
    /* Start PeriodicTask without the 1st update:
     * DynamicConfig has been initialized with
     * config cached in FS. Components loading will continue.
     */
    StartPeriodicUpdates(Flag::kNoFirstUpdate);
  }
}

DynamicConfigClientUpdater::~DynamicConfigClientUpdater() {
  StopPeriodicUpdates();
}

void DynamicConfigClientUpdater::StoreIfEnabled() {
  auto ptr = Get();
  if (store_enabled_) updates_sink_.SetConfig(kName, *ptr);
  CheckUnusedConfigs(*ptr, docs_map_keys_);
}

std::vector<std::string> DynamicConfigClientUpdater::GetDocsMapKeysToFetch(
    AdditionalDocsMapKeys& additional_docs_map_keys) {
  if (load_only_my_values_) {
    auto docs_map_keys = docs_map_keys_;

    for (auto it = additional_docs_map_keys.begin();
         it != additional_docs_map_keys.end();) {
      // Use reference counting to make sure that all consumers of config keys
      // may .Get() their keys. We may not guarantee that by any synchronization
      // on Update()/SetAdditionalKeys() as consumers may outlive these calls
      // and die well after Update()/SetAdditionalKeys() return.
      if (it->use_count() > 1) {
        UASSERT(*it);
        for (const auto& additional_key : **it) {
          docs_map_keys.insert(additional_key);
        }
        ++it;
      } else {
        it = additional_docs_map_keys.erase(it);
      }
    }

    return {docs_map_keys.begin(), docs_map_keys.end()};
  }

  return {};
}

dynamic_config::AdditionalKeysToken
DynamicConfigClientUpdater::SetAdditionalKeys(std::vector<std::string> keys) {
  if (!load_only_my_values_ || keys.empty())
    return dynamic_config::AdditionalKeysToken{nullptr};

  auto keys_ptr = std::make_shared<std::vector<std::string>>(std::move(keys));
  {
    auto additional_docs_map_keys = additional_docs_map_keys_.Lock();
    additional_docs_map_keys->insert(keys_ptr);
  }
  UpdateAdditionalKeys(*keys_ptr);
  return dynamic_config::AdditionalKeysToken{std::move(keys_ptr)};
}

void DynamicConfigClientUpdater::Update(
    cache::UpdateType update_type,
    const std::chrono::system_clock::time_point& /*last_update*/,
    const std::chrono::system_clock::time_point& /*now*/,
    cache::UpdateStatisticsScope& stats) {
  auto additional_docs_map_keys = additional_docs_map_keys_.Lock();
  if (update_type == cache::UpdateType::kFull) {
    auto reply = config_client_.FetchDocsMap(
        std::nullopt, GetDocsMapKeysToFetch(*additional_docs_map_keys));
    auto& docs_map = reply.docs_map;

    stats.IncreaseDocumentsReadCount(docs_map.Size());

    /* Don't check for timestamp, accept any timestamp.
     * Otherwise we might end up with constantly failing to make full update
     * as every full update we get a bit outdated reply.
     */

    dynamic_config::DocsMap combined(fallback_config_);
    combined.MergeFromOther(std::move(docs_map));

    auto size = combined.Size();
    {
      std::lock_guard<engine::Mutex> lock(update_config_mutex_);
      if (IsDuplicate(update_type, combined)) {
        stats.FinishNoChanges();
        server_timestamp_ = reply.timestamp;
        return;
      }
      Set(std::move(combined));
      StoreIfEnabled();
    }

    stats.Finish(size);
    server_timestamp_ = reply.timestamp;
  } else {
    // kIncremental
    auto reply = config_client_.FetchDocsMap(
        server_timestamp_, GetDocsMapKeysToFetch(*additional_docs_map_keys));
    auto& docs_map = reply.docs_map;

    /* Timestamp can be compared lexicographically */
    if (reply.timestamp < server_timestamp_) {
      stats.FinishNoChanges();
      return;
    }

    if (reply.docs_map.Size() == 0) {
      stats.FinishNoChanges();
      server_timestamp_ = reply.timestamp;
      return;
    }

    stats.IncreaseDocumentsReadCount(docs_map.Size());

    {
      std::lock_guard<engine::Mutex> lock(update_config_mutex_);
      auto ptr = Get();
      dynamic_config::DocsMap combined = *ptr;
      combined.MergeFromOther(std::move(docs_map));

      if (IsDuplicate(update_type, combined)) {
        stats.FinishNoChanges();
        server_timestamp_ = reply.timestamp;
        return;
      }

      auto size = combined.Size();
      Emplace(std::move(combined));
      StoreIfEnabled();

      stats.Finish(size);
    }
    server_timestamp_ = reply.timestamp;
  }
}

void DynamicConfigClientUpdater::UpdateAdditionalKeys(
    const std::vector<std::string>& keys) {
  auto reply = config_client_.FetchDocsMap(std::nullopt, keys);
  auto& combined = reply.docs_map;

  {
    std::lock_guard<engine::Mutex> lock(update_config_mutex_);
    auto ptr = Get();
    dynamic_config::DocsMap docs_map = *ptr;
    combined.MergeFromOther(std::move(docs_map));

    Emplace(std::move(combined));
    StoreIfEnabled();
  }
}

bool DynamicConfigClientUpdater::IsDuplicate(
    cache::UpdateType update_type,
    const dynamic_config::DocsMap& new_value) const {
  if (ShouldDeduplicate(deduplicate_update_types_, update_type)) {
    if (const auto old_config = GetUnsafe();
        old_config && old_config->AreContentsEqual(new_value)) {
      return true;
    }
  }
  return false;
}

yaml_config::Schema DynamicConfigClientUpdater::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<CachingComponentBase>(R"(
type: object
description: Component that does a periodic update of runtime configs.
additionalProperties: false
properties:
    updates-sink:
        type: string
        description: components::DynamicConfigUpdatesSinkBase descendant to be used for storing received updates
        defaultDescription: dynamic-config
    store-enabled:
        type: boolean
        description: store the retrieved values into the updates sink component
    load-only-my-values:
        type: boolean
        description: request from the client only the values used by this service
    fallback-path:
        type: string
        description: a path to the fallback config to load the required config names from it
    fs-task-processor:
        type: string
        description: name of the task processor to run the blocking file write operations
    deduplicate-update-types:
        type: string
        description: config update types for best-effort deduplication
        defaultDescription: full-and-incremental
        enum:
          - none
          - only-full
          - only-incremental
          - full-and-incremental
)");
}

}  // namespace components

USERVER_NAMESPACE_END
