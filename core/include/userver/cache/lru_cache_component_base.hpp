#pragma once

/// @file userver/cache/lru_cache_component_base.hpp
/// @brief @copybrief cache::LruCacheComponent

#include <functional>

#include <userver/cache/expirable_lru_cache.hpp>
#include <userver/cache/lru_cache_config.hpp>
#include <userver/components/loggable_component_base.hpp>
#include <userver/concurrent/async_event_source.hpp>
#include <userver/dump/dumper.hpp>
#include <userver/dump/meta.hpp>
#include <userver/dump/operations.hpp>
#include <userver/dynamic_config/source.hpp>
#include <userver/logging/log.hpp>
#include <userver/testsuite/component_control.hpp>
#include <userver/utils/statistics/entry.hpp>
#include <userver/yaml_config/schema.hpp>

USERVER_NAMESPACE_BEGIN

namespace cache {

namespace impl {

testsuite::ComponentControl& FindComponentControl(
    const components::ComponentContext& context);

utils::statistics::Entry RegisterOnStatisticsStorage(
    const components::ComponentContext& context, const std::string& name,
    std::function<void(utils::statistics::Writer&)> func);

dynamic_config::Source FindDynamicConfigSource(
    const components::ComponentContext& context);

bool IsDumpSupportEnabled(const components::ComponentConfig& config);

yaml_config::Schema GetLruCacheComponentBaseSchema();

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
// NOLINTNEXTLINE(fuchsia-multiple-inheritance)
class LruCacheComponent : public components::LoggableComponentBase,
                          private dump::DumpableEntity {
 public:
  using Cache = ExpirableLruCache<Key, Value, Hash, Equal>;
  using CacheWrapper = LruCacheWrapper<Key, Value, Hash, Equal>;

  LruCacheComponent(const components::ComponentConfig&,
                    const components::ComponentContext&);

  ~LruCacheComponent() override;

  CacheWrapper GetCache();

  static yaml_config::Schema GetStaticConfigSchema();

 protected:
  virtual Value DoGetByKey(const Key& key) = 0;

 private:
  void DropCache();

  Value GetByKey(const Key& key);

  void OnConfigUpdate(const dynamic_config::Snapshot& cfg);

  void UpdateConfig(const LruCacheConfig& config);

  static constexpr bool kCacheIsDumpable =
      dump::kIsDumpable<Key> && dump::kIsDumpable<Value>;

  void GetAndWrite(dump::Writer& writer) const override;
  void ReadAndSet(dump::Reader& reader) override;

 protected:
  std::shared_ptr<Cache> GetCacheRaw() { return cache_; }

 private:
  const std::string name_;
  const LruCacheConfigStatic static_config_;
  std::shared_ptr<dump::Dumper> dumper_;
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
  if (impl::IsDumpSupportEnabled(config)) {
    dumper_ = std::make_shared<dump::Dumper>(
        config, context, static_cast<dump::DumpableEntity&>(*this));
    cache_->SetDumper(dumper_);
    dumper_->ReadDump();
  }

  cache_->SetMaxLifetime(static_config_.config.lifetime);
  cache_->SetBackgroundUpdate(static_config_.config.background_update);

  if (static_config_.use_dynamic_config) {
    LOG_INFO() << "Dynamic LRU cache config is enabled, subscribing on "
                  "dynamic-config updates, cache="
               << name_;

    config_subscription_ =
        impl::FindDynamicConfigSource(context).UpdateAndListen(
            this, "cache." + name_,
            &LruCacheComponent<Key, Value, Hash, Equal>::OnConfigUpdate);
  } else {
    LOG_INFO() << "Dynamic LRU cache config is disabled, cache=" << name_;
  }

  statistics_holder_ = impl::RegisterOnStatisticsStorage(
      context, name_,
      [this](utils::statistics::Writer& writer) { writer = *cache_; });

  invalidator_holder_.emplace(
      impl::FindComponentControl(context), *this,
      &LruCacheComponent<Key, Value, Hash, Equal>::DropCache);
}

template <typename Key, typename Value, typename Hash, typename Equal>
LruCacheComponent<Key, Value, Hash, Equal>::~LruCacheComponent() {
  invalidator_holder_.reset();
  statistics_holder_.Unregister();
  config_subscription_.Unsubscribe();

  if (dumper_) {
    dumper_->CancelWriteTaskAndWait();
  }
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

template <typename Key, typename Value, typename Hash, typename Equal>
yaml_config::Schema
LruCacheComponent<Key, Value, Hash, Equal>::GetStaticConfigSchema() {
  return impl::GetLruCacheComponentBaseSchema();
}

template <typename Key, typename Value, typename Hash, typename Equal>
void LruCacheComponent<Key, Value, Hash, Equal>::GetAndWrite(
    dump::Writer& writer) const {
  if constexpr (kCacheIsDumpable) {
    cache_->Write(writer);
  } else {
    dump::ThrowDumpUnimplemented(name_);
  }
}

template <typename Key, typename Value, typename Hash, typename Equal>
void LruCacheComponent<Key, Value, Hash, Equal>::ReadAndSet(
    dump::Reader& reader) {
  if constexpr (kCacheIsDumpable) {
    cache_->Read(reader);
  } else {
    dump::ThrowDumpUnimplemented(name_);
  }
}

}  // namespace cache

USERVER_NAMESPACE_END
