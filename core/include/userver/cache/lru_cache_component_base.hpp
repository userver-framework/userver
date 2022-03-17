#pragma once

/// @file userver/cache/lru_cache_component_base.hpp
/// @brief @copybrief cache::LruCacheComponent

#include <userver/cache/expirable_lru_cache.hpp>
#include <userver/cache/lru_cache_config.hpp>
#include <userver/components/loggable_component_base.hpp>
#include <userver/concurrent/async_event_source.hpp>
#include <userver/dynamic_config/source.hpp>
#include <userver/testsuite/component_control.hpp>
#include <userver/utils/statistics/storage.hpp>

// TODO remove extra includes
#include <userver/components/component_context.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/concurrent/async_event_channel.hpp>
#include <userver/taxi_config/storage/component.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/statistics/metadata.hpp>

USERVER_NAMESPACE_BEGIN

namespace cache {

namespace impl {

formats::json::Value GetCacheStatisticsAsJson(
    const ExpirableLruCacheStatistics& stats, std::size_t size);

template <typename Key, typename Value, typename Hash, typename Equal>
formats::json::Value GetCacheStatisticsAsJson(
    const ExpirableLruCache<Key, Value, Hash, Equal>& cache) {
  return GetCacheStatisticsAsJson(cache.GetStatistics(),
                                  cache.GetSizeApproximate());
}

testsuite::ComponentControl& FindComponentControl(
    const components::ComponentContext& context);

utils::statistics::Storage& FindStatisticsStorage(
    const components::ComponentContext& context);

dynamic_config::Source FindDynamicConfigSource(
    const components::ComponentContext& context);

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
/// and may be reconfigured dynamically via components::DynamicConfig.
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
///
/// ## Example usage:
///
/// @snippet cache/lru_cache_component_base_test.hpp  Sample lru cache component
///
/// Do not forget to @ref userver_components "add the component to component list":
/// @snippet cache/lru_cache_component_base_test.cpp  Sample lru cache component registration
///
/// ## Example config:
/// @snippet cache/lru_cache_component_base_test.cpp  Sample lru cache component config

// clang-format on
template <typename Key, typename Value, typename Hash = std::hash<Key>,
          typename Equal = std::equal_to<Key>>
class LruCacheComponent : public components::LoggableComponentBase {
 public:
  using Cache = ExpirableLruCache<Key, Value, Hash, Equal>;
  using CacheWrapper = LruCacheWrapper<Key, Value, Hash, Equal>;

  LruCacheComponent(const components::ComponentConfig&,
                    const components::ComponentContext&);

  ~LruCacheComponent() override;

  CacheWrapper GetCache();

 protected:
  virtual Value DoGetByKey(const Key& key) = 0;

 private:
  void DropCache();

  Value GetByKey(const Key& key);

  void OnConfigUpdate(const dynamic_config::Snapshot& cfg);

  void UpdateConfig(const LruCacheConfig& config);

 protected:
  std::shared_ptr<Cache> GetCacheRaw() { return cache_; }

 private:
  const std::string name_;
  const LruCacheConfigStatic static_config_;
  const std::shared_ptr<Cache> cache_;
  concurrent::AsyncEventSubscriberScope config_subscription_;
  utils::statistics::Entry statistics_holder_;
  std::optional<testsuite::ComponentInvalidatorHolder> invalidator_holder_;
};

template <typename Key, typename Value, typename Hash, typename Equal>
LruCacheComponent<Key, Value, Hash, Equal>::LruCacheComponent(
    const components::ComponentConfig& config,
    const components::ComponentContext& context)
    : LoggableComponentBase(config, context),
      name_(components::GetCurrentComponentName(config)),
      static_config_(config),
      cache_(std::make_shared<Cache>(static_config_.ways,
                                     static_config_.GetWaySize())) {
  cache_->SetMaxLifetime(static_config_.config.lifetime);
  cache_->SetBackgroundUpdate(static_config_.config.background_update);

  if (static_config_.use_dynamic_config) {
    LOG_INFO() << "Dynamic LRU cache config is enabled, subscribing on "
                  "taxi-config updates, cache="
               << name_;

    config_subscription_ =
        impl::FindDynamicConfigSource(context).UpdateAndListen(
            this, "cache." + name_,
            &LruCacheComponent<Key, Value, Hash, Equal>::OnConfigUpdate);
  } else {
    LOG_INFO() << "Dynamic LRU cache config is disabled, cache=" << name_;
  }

  statistics_holder_ = impl::FindStatisticsStorage(context).RegisterExtender(
      "cache." + name_, [this](const auto& /*request*/) {
        return impl::GetCacheStatisticsAsJson(*cache_);
      });

  invalidator_holder_.emplace(
      impl::FindComponentControl(context), *this,
      &LruCacheComponent<Key, Value, Hash, Equal>::DropCache);
}

template <typename Key, typename Value, typename Hash, typename Equal>
LruCacheComponent<Key, Value, Hash, Equal>::~LruCacheComponent() {
  invalidator_holder_.reset();
  statistics_holder_.Unregister();
  config_subscription_.Unsubscribe();
}

template <typename Key, typename Value, typename Hash, typename Equal>
typename LruCacheComponent<Key, Value, Hash, Equal>::CacheWrapper
LruCacheComponent<Key, Value, Hash, Equal>::GetCache() {
  return CacheWrapper(cache_, [this](const Key& key) { return GetByKey(key); });
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
    const dynamic_config::Snapshot& cfg) {
  const auto config = GetLruConfig(cfg, name_);
  if (config) {
    LOG_DEBUG() << "Using dynamic config for LRU cache";
    UpdateConfig(*config);
  } else {
    LOG_DEBUG() << "Using static config for LRU cache";
    UpdateConfig(static_config_.config);
  }
}

template <typename Key, typename Value, typename Hash, typename Equal>
void LruCacheComponent<Key, Value, Hash, Equal>::UpdateConfig(
    const LruCacheConfig& config) {
  cache_->SetWaySize(config.GetWaySize(static_config_.ways));
  cache_->SetMaxLifetime(config.lifetime);
  cache_->SetBackgroundUpdate(config.background_update);
}

}  // namespace cache

USERVER_NAMESPACE_END
