#include <userver/dynamic_config/updater/component.hpp>

#include <userver/cache/update_type.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/components/component.hpp>
#include <userver/dynamic_config/client/component.hpp>
#include <userver/dynamic_config/storage/component.hpp>
#include <userver/dynamic_config/updates_sink/find.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/fs/read.hpp>
#include <userver/utils/algo.hpp>
#include <userver/utils/string_to_duration.hpp>
#include <userver/yaml_config/merge_schemas.hpp>
#include <utils/internal_tag.hpp>

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

const ComponentConfig& EnsureDoesNotDependOnDynamicConfig(
    const ComponentConfig& component_config) {
  UINVARIANT(!component_config["config-settings"].As<bool>(true),
             "dynamic-config-client-updater.config-settings must be false, "
             "otherwise it will create a cyclic dependency of components");
  return component_config;
}

}  // namespace

DynamicConfigClientUpdater::DynamicConfigClientUpdater(
    const ComponentConfig& component_config,
    const ComponentContext& component_context)
    : CachingComponentBase(EnsureDoesNotDependOnDynamicConfig(component_config),
                           component_context),
      updates_sink_(
          dynamic_config::FindUpdatesSink(component_config, component_context)),
      load_only_my_values_(
          component_config["load-only-my-values"].As<bool>(true)),
      store_enabled_(component_config["store-enabled"].As<bool>(true)),
      deduplicate_update_types_(ParseDeduplicateUpdateTypes(
          component_config["deduplicate-update-types"])),
      config_client_(
          component_context.FindComponent<components::DynamicConfigClient>()
              .GetClient()),
      docs_map_keys_(utils::AsContainer<DocsMapKeys>(
          component_context.FindComponent<components::DynamicConfig>()
              .GetDefaultDocsMap()
              .GetNames())) {
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

dynamic_config::DocsMap DynamicConfigClientUpdater::MergeDocsMap(
    const dynamic_config::DocsMap& current, dynamic_config::DocsMap&& update) {
  dynamic_config::DocsMap combined(std::move(update));
  combined.MergeMissing(current);
  combined.SetConfigsExpectedToBeUsed(docs_map_keys_, utils::InternalTag{});
  return combined;
}

void DynamicConfigClientUpdater::StoreIfEnabled() {
  if (store_enabled_) {
    auto ptr = Get();
    updates_sink_.SetConfig(kName, *ptr);
  }
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

    // Don't check for timestamp, accept any timestamp.
    // Otherwise, we might end up with constantly failing to make full update
    // as every full update we get a bit outdated reply.

    const auto size = docs_map.Size();

    {
      const std::lock_guard lock(update_config_mutex_);
      if (IsDuplicate(update_type, docs_map)) {
        stats.FinishNoChanges();
        server_timestamp_ = reply.timestamp;
        return;
      }

      Set(std::move(docs_map));
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
      const std::lock_guard lock(update_config_mutex_);
      auto ptr = Get();
      auto combined = MergeDocsMap(*ptr, std::move(docs_map));

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
  auto& additional_configs = reply.docs_map;

  {
    const std::lock_guard lock(update_config_mutex_);
    auto ptr = Get();
    dynamic_config::DocsMap docs_map = *ptr;
    auto combined = MergeDocsMap(additional_configs, std::move(docs_map));

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
        defaultDescription: true
    load-only-my-values:
        type: boolean
        description: request from the client only the values used by this service
        defaultDescription: true
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
