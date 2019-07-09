#pragma once

#include <cache/cache_config.hpp>
#include <cache/expirable_lru_cache.hpp>
#include <components/component_base.hpp>
#include <components/statistics_storage.hpp>
#include <server/cache_invalidator_holder.hpp>
#include <taxi_config/storage/component.hpp>
#include <utils/async_event_channel.hpp>
#include <utils/statistics/metadata.hpp>
#include <utils/statistics/storage.hpp>

namespace cache {

namespace impl {

template <typename Key, typename Value>
formats::json::Value GetCacheStatisticsAsJson(
    const ExpirableLruCache<Key, Value>& cache) {
  formats::json::ValueBuilder builder;
  utils::statistics::SolomonLabelValue(builder, "cache_name");

  auto& stats = cache.GetStatistics();
  builder[kStatisticsNameCurrentDocumentsCount] = cache.GetSizeApproximate();
  builder[kStatisticsNameHits] = stats.hits.load();
  builder[kStatisticsNameMisses] = stats.misses.load();
  return builder.ExtractValue();
}

}  // namespace impl

template <typename Key, typename Value>
class LruCacheComponent : public components::LoggableComponentBase {
 public:
  using Cache = ExpirableLruCache<Key, Value>;
  using CacheWrapper = LruCacheWrapper<Key, Value>;

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

  void OnConfigUpdate(const std::shared_ptr<taxi_config::Config>& cfg);

  void UpdateConfig(const LruCacheConfigStatic& config);

 private:
  const std::string name_;
  utils::statistics::Entry statistics_holder_;
  const LruCacheConfigStatic static_config_;
  std::shared_ptr<Cache> cache_;
  utils::AsyncEventSubscriberScope config_subscription_;
  server::ComponentInvalidatorHolder<LruCacheComponent<Key, Value>>
      invalidator_holder_;
};

template <typename Key, typename Value>
LruCacheComponent<Key, Value>::LruCacheComponent(
    const components::ComponentConfig& config,
    const components::ComponentContext& context)
    : LoggableComponentBase(config, context),
      name_(config.Name()),
      static_config_(config),
      cache_(std::make_shared<Cache>(static_config_.ways,
                                     static_config_.GetWaySize())),
      invalidator_holder_(*this, context,
                          &LruCacheComponent<Key, Value>::DropCache) {
  cache_->UpdateMaxLifetime(static_config_.config.lifetime);

  auto& storage =
      context.FindComponent<components::StatisticsStorage>().GetStorage();
  statistics_holder_ = storage.RegisterExtender(
      "cache." + name_,
      [this](const utils::statistics::StatisticsRequest& request) {
        return ExtendStatistics(request);
      });

  if (config.ParseBool("config-settings", true) &&
      CacheConfigSet::IsLruConfigEnabled()) {
    LOG_INFO() << "Dynamic LRU cache config is enabled, subscribing on "
                  "taxi-config updates, cache="
               << name_;

    auto& taxi_config = context.FindComponent<components::TaxiConfig>();
    OnConfigUpdate(taxi_config.Get());
    config_subscription_ = taxi_config.AddListener(
        this, "cache-" + name_, &LruCacheComponent<Key, Value>::OnConfigUpdate);
  } else {
    LOG_INFO() << "Dynamic LRU cache config is disabled, cache=" << name_;
  }
}

template <typename Key, typename Value>
LruCacheComponent<Key, Value>::~LruCacheComponent() {
  config_subscription_.Unsubscribe();
  statistics_holder_.Unregister();
}

template <typename Key, typename Value>
typename LruCacheComponent<Key, Value>::CacheWrapper
LruCacheComponent<Key, Value>::GetCache() {
  return CacheWrapper(cache_, [this](const Key& key) { return GetByKey(key); });
}

template <typename Key, typename Value>
formats::json::Value LruCacheComponent<Key, Value>::ExtendStatistics(
    const utils::statistics::StatisticsRequest&) {
  return impl::GetCacheStatisticsAsJson(*cache_);
}

template <typename Key, typename Value>
void LruCacheComponent<Key, Value>::DropCache() {
  cache_->Invalidate();
}

template <typename Key, typename Value>
Value LruCacheComponent<Key, Value>::GetByKey(const Key& key) {
  return DoGetByKey(key);
}

template <typename Key, typename Value>
void LruCacheComponent<Key, Value>::OnConfigUpdate(
    const std::shared_ptr<taxi_config::Config>& cfg) {
  auto config = cfg->Get<CacheConfigSet>().GetLruConfig(name_);
  if (config) {
    LOG_DEBUG() << "Using dymanic config for LRU cache";
    UpdateConfig(static_config_.MergeWith(*config));
  } else {
    LOG_DEBUG() << "Using static config for LRU cache";
    UpdateConfig(static_config_);
  }
}

template <typename Key, typename Value>
void LruCacheComponent<Key, Value>::UpdateConfig(
    const LruCacheConfigStatic& config) {
  cache_->UpdateWaySize(config.GetWaySize());
  cache_->UpdateMaxLifetime(config.config.lifetime);
}

}  // namespace cache
