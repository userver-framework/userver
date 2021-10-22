#include <userver/taxi_config/updater/client/component.hpp>

#include <userver/cache/update_type.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/fs/blocking/read.hpp>
#include <userver/taxi_config/configs/component.hpp>
#include <userver/utils/string_to_duration.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

TaxiConfigClientUpdater::TaxiConfigClientUpdater(
    const ComponentConfig& component_config,
    const ComponentContext& component_context)
    : CachingComponentBase(component_config, component_context),
      updater_(component_context),
      load_only_my_values_(component_config["load-only-my-values"].As<bool>()),
      store_enabled_(component_config["store-enabled"].As<bool>()),
      config_client_(
          component_context.FindComponent<components::TaxiConfigClient>()
              .GetClient()) {
  auto fallback_config_contents = fs::blocking::ReadFileContents(
      component_config["fallback-path"].As<std::string>());
  try {
    fallback_config_.Parse(fallback_config_contents, false);

    // There are all required configs in the fallbacks file
    auto docs_map_keys = docs_map_keys_.Lock();
    *docs_map_keys = fallback_config_.GetNames();
  } catch (const std::exception& ex) {
    throw std::runtime_error(std::string("Cannot load fallback taxi config: ") +
                             ex.what());
  }

  try {
    StartPeriodicUpdates();
  } catch (const std::exception& e) {
    LOG_ERROR() << "Config client updater initialization failed: " << e;
    updater_.NotifyLoadingFailed(e.what());
    /* Start PeriodicTask without the 1st update:
     * TaxiConfig has been initialized with
     * config cached in FS. Components loading will continue.
     */
    StartPeriodicUpdates(Flag::kNoFirstUpdate);
  }
}

TaxiConfigClientUpdater::~TaxiConfigClientUpdater() { StopPeriodicUpdates(); }

void TaxiConfigClientUpdater::StoreIfEnabled() {
  auto ptr = Get();
  if (store_enabled_) updater_.SetConfig(*ptr);

  auto docs_map_keys = docs_map_keys_.Lock();
  *docs_map_keys = ptr->GetRequestedNames();
}

TaxiConfigClientUpdater::DocsMapKeys
TaxiConfigClientUpdater::GetStoredDocsMapKeys() const {
  auto docs_map_keys = docs_map_keys_.Lock();
  return *docs_map_keys;
}

std::vector<std::string> TaxiConfigClientUpdater::GetDocsMapKeysToFetch(
    AdditionalDocsMapKeys& additional_docs_map_keys) {
  if (load_only_my_values_) {
    auto docs_map_keys = GetStoredDocsMapKeys();

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

taxi_config::AdditionalKeysToken TaxiConfigClientUpdater::SetAdditionalKeys(
    std::vector<std::string> keys) {
  if (!load_only_my_values_ || keys.empty())
    return taxi_config::AdditionalKeysToken{nullptr};

  auto keys_ptr = std::make_shared<std::vector<std::string>>(std::move(keys));
  {
    auto additional_docs_map_keys = additional_docs_map_keys_.Lock();
    additional_docs_map_keys->insert(keys_ptr);
  }
  UpdateAdditionalKeys(*keys_ptr);
  return taxi_config::AdditionalKeysToken{std::move(keys_ptr)};
}

void TaxiConfigClientUpdater::Update(
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

    taxi_config::DocsMap combined(fallback_config_);
    combined.MergeFromOther(std::move(docs_map));

    auto size = combined.Size();
    {
      std::lock_guard<engine::Mutex> lock(update_config_mutex_);
      Emplace(std::move(combined));
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
      return;
    }

    stats.IncreaseDocumentsReadCount(docs_map.Size());

    {
      std::lock_guard<engine::Mutex> lock(update_config_mutex_);
      taxi_config::DocsMap combined = *Get();
      combined.MergeFromOther(std::move(docs_map));

      auto size = combined.Size();
      Emplace(std::move(combined));
      StoreIfEnabled();

      stats.Finish(size);
    }
    server_timestamp_ = reply.timestamp;
  }
}

void TaxiConfigClientUpdater::UpdateAdditionalKeys(
    const std::vector<std::string>& keys) {
  auto reply = config_client_.FetchDocsMap(std::nullopt, keys);
  auto& combined = reply.docs_map;

  {
    std::lock_guard<engine::Mutex> lock(update_config_mutex_);
    taxi_config::DocsMap docs_map = *Get();
    combined.MergeFromOther(std::move(docs_map));

    Emplace(std::move(combined));
    StoreIfEnabled();
  }
}

}  // namespace components

USERVER_NAMESPACE_END
