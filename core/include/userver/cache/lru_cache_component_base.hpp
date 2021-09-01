#pragma once

/// @file userver/cache/lru_cache_component_base.hpp
/// @brief @copybrief components::LruCacheComponent

#include <userver/cache/cache_config.hpp>
#include <userver/cache/expirable_lru_cache.hpp>
#include <userver/cache/lru_cache_config.hpp>
#include <userver/components/loggable_component_base.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/concurrent/async_event_channel.hpp>
#include <userver/taxi_config/storage/component.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/statistics/metadata.hpp>
#include <userver/utils/statistics/storage.hpp>

namespace cache {

namespace impl {

template <typename Key, typename Value, typename Hash, typename Equal>
formats::json::Value GetCacheStatisticsAsJson(
    const ExpirableLruCache<Key, Value, Hash, Equal>& cache) {
  formats::json::ValueBuilder builder;
  utils::statistics::SolomonLabelValue(builder, "cache_name");

  auto& stats = cache.GetStatistics();
  builder[kStatisticsNameCurrentDocumentsCount] = cache.GetSizeApproximate();
  builder[kStatisticsNameHits] = stats.total.hits.load();
  builder[kStatisticsNameMisses] = stats.total.misses.load();
  builder[kStatisticsNameStale] = stats.total.stale.load();
  builder[kStatisticsNameBackground] = stats.total.background_updates.load();

  auto s1min = stats.recent.GetStatsForPeriod();
  double s1min_hits = s1min.hits.load();
  auto s1min_total = s1min.hits.load() + s1min.misses.load();
  builder[kStatisticsNameHitRatio]["1min"] =
      s1min_hits / (s1min_total ? s1min_total : 1);
  return builder.ExtractValue();
}

}  // namespace impl

// clang-format off

/// @ingroup userver_components userver_base_classes
///
/// @brief Base class for LRU-cache components
///
/// Provides facilities for creating LRU caches.
/// You need to override LruCacheComponent::DoGetByKey to handle cache misses.
///
/// Caching components must be configured in service config (see options below)
/// and may be reconfigured dynamically via components::TaxiConfig.
///
/// ## Dynamic config
/// * @ref USERVER_LRU_CACHES
///
/// ## Static options:
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// size | max amount of items to store in cache | --
/// ways | number of ways for associative cache | --
/// lifetime | TTL for cache entries (0 is unlimited) | 0
/// config-settings | enables dynamic reconfiguration with CacheConfigSet | true

// clang-format on
template <typename Key, typename Value, typename Hash = std::hash<Key>,
          typename Equal = std::equal_to<Key>>
class LruCacheComponent : public components::LoggableComponentBase {
 public:
  using Cache = ExpirableLruCache<Key, Value, Hash, Equal>;
  using CacheWrapper = LruCacheWrapper<Key, Value, Hash, Equal>;

  LruCacheComponent(const components::ComponentConfig&,
                    const components::ComponentContext&);

  ~LruCacheComponent();

  CacheWrapper GetCache();

 protected:
  virtual Value DoGetByKey(const Key& key) = 0;

 private:
  static std::shared_ptr<Cache> CreateCache(
      const components::ComponentConfig& config);

  formats::json::Value ExtendStatistics(
      const utils::statistics::StatisticsRequest& /*request*/);

  void DropCache();

  Value GetByKey(const Key& key);

  void OnConfigUpdate(const taxi_config::Snapshot& cfg);

  void UpdateConfig(const LruCacheConfigStatic& config);

 protected:
  std::shared_ptr<Cache> GetCacheRaw() { return cache_; }

 private:
  const std::string name_;
  utils::statistics::Entry statistics_holder_;
  const LruCacheConfigStatic static_config_;
  std::shared_ptr<Cache> cache_;
  concurrent::AsyncEventSubscriberScope config_subscription_;
  testsuite::ComponentInvalidatorHolder invalidator_holder_;
};

template <typename Key, typename Value, typename Hash, typename Equal>
LruCacheComponent<Key, Value, Hash, Equal>::LruCacheComponent(
    const components::ComponentConfig& config,
    const components::ComponentContext& context)
    : LoggableComponentBase(config, context),
      name_(config.Name()),
      static_config_(config),
      cache_(std::make_shared<Cache>(static_config_.ways,
                                     static_config_.GetWaySize())),
      invalidator_holder_(
          context.FindComponent<components::TestsuiteSupport>()
              .GetComponentControl(),
          *this, &LruCacheComponent<Key, Value, Hash, Equal>::DropCache) {
  cache_->SetMaxLifetime(static_config_.config.lifetime);
  cache_->SetBackgroundUpdate(static_config_.config.background_update);

  auto& storage =
      context.FindComponent<components::StatisticsStorage>().GetStorage();
  statistics_holder_ = storage.RegisterExtender(
      "cache." + name_,
      [this](const utils::statistics::StatisticsRequest& request) {
        return ExtendStatistics(request);
      });

  if (config["config-settings"].As<bool>(true)) {
    LOG_INFO() << "Dynamic LRU cache config is enabled, subscribing on "
                  "taxi-config updates, cache="
               << name_;

    config_subscription_ =
        context.FindComponent<components::TaxiConfig>()
            .GetSource()
            .UpdateAndListen(
                this, "cache." + name_,
                &LruCacheComponent<Key, Value, Hash, Equal>::OnConfigUpdate);
  } else {
    LOG_INFO() << "Dynamic LRU cache config is disabled, cache=" << name_;
  }
}

template <typename Key, typename Value, typename Hash, typename Equal>
LruCacheComponent<Key, Value, Hash, Equal>::~LruCacheComponent() {
  config_subscription_.Unsubscribe();
  statistics_holder_.Unregister();
}

template <typename Key, typename Value, typename Hash, typename Equal>
typename LruCacheComponent<Key, Value, Hash, Equal>::CacheWrapper
LruCacheComponent<Key, Value, Hash, Equal>::GetCache() {
  return CacheWrapper(cache_, [this](const Key& key) { return GetByKey(key); });
}

template <typename Key, typename Value, typename Hash, typename Equal>
formats::json::Value
LruCacheComponent<Key, Value, Hash, Equal>::ExtendStatistics(
    const utils::statistics::StatisticsRequest&) {
  return impl::GetCacheStatisticsAsJson(*cache_);
}

template <typename Key, typename Value, typename Hash, typename Equal>
void LruCacheComponent<Key, Value, Hash, Equal>::DropCache() {
  cache_->Invalidate();
}

template <typename Key, typename Value, typename Hash, typename Equal>
Value LruCacheComponent<Key, Value, Hash, Equal>::GetByKey(const Key& key) {
  return DoGetByKey(key);
}

template <typename Key, typename Value, typename Hash, typename Equal>
void LruCacheComponent<Key, Value, Hash, Equal>::OnConfigUpdate(
    const taxi_config::Snapshot& cfg) {
  const auto config = GetLruConfig(cfg, name_);
  if (config) {
    LOG_DEBUG() << "Using dynamic config for LRU cache";
    UpdateConfig(static_config_.MergeWith(*config));
  } else {
    LOG_DEBUG() << "Using static config for LRU cache";
    UpdateConfig(static_config_);
  }
}

template <typename Key, typename Value, typename Hash, typename Equal>
void LruCacheComponent<Key, Value, Hash, Equal>::UpdateConfig(
    const LruCacheConfigStatic& config) {
  cache_->SetWaySize(config.GetWaySize());
  cache_->SetMaxLifetime(config.config.lifetime);
  cache_->SetBackgroundUpdate(config.config.background_update);
}

}  // namespace cache
