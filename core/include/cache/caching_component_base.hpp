#pragma once

#include <atomic>
#include <memory>
#include <mutex>

#include <cache/cache_statistics.hpp>
#include <components/statistics_storage.hpp>
#include <engine/condition_variable.hpp>
#include <server/cache_invalidator_holder.hpp>
#include <taxi_config/storage/component.hpp>
#include <utils/async_event_channel.hpp>
#include <utils/statistics/metadata.hpp>
#include <utils/swappingsmart.hpp>

#include <components/component_base.hpp>
#include <components/component_config.hpp>
#include "cache_config.hpp"
#include "cache_update_trait.hpp"

namespace components {

// clang-format off

/// @brief Base class for caching components
///
/// Provides facilities for creating periodically updated caches.
/// You need to override CacheUpdateTrait::Update
/// then call CacheUpdateTrait::StartPeriodicUpdates after setup
/// and CacheUpdateTrait::StopPeriodicUpdates before teardown.
/// 
/// Caching components must be configured in service config (see options below)
/// and may be reconfigured dynamically via TaxiConfig.
///
/// ## Available options:
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// update-types | specifies whether incremental and/or full updates will be used | see below
/// update-interval | (*required*) interval between Update invocations | --
/// update-jitter | max. amount of time by which interval may be adjusted for requests desynchronization | update_interval / 10
/// full-update-interval | interval between full updates | --
/// config-settings | enables dynamic reconfiguration with CacheConfigSet | true
///
/// ### Update types
///  * `full-and-incremental`: both `update-interval` and `full-update-interval`
///    must be specified. Updates with UpdateType::kIncremental will be triggered
///    each `update-interval` (adjusted by jitter) unless `full-update-interval`
///    has passed and UpdateType::kFull is triggered.
///  * `only-full`: only `update-interval` must be specified. UpdateType::kFull
///    will be triggered each `update-interval` (adjusted by jitter).
///
/// By default, update types are guessed based on update intervals presence.
/// If both `update-interval` and `full-update-interval` are present,
/// `full-and-incremental` types is assumed. Otherwise `only-full` is used.

// clang-format on

template <typename T>
class CachingComponentBase
    : public LoggableComponentBase,
      public utils::AsyncEventChannel<const std::shared_ptr<const T>&>,
      protected CacheUpdateTrait {
 public:
  CachingComponentBase(const ComponentConfig& config, const ComponentContext&,
                       const std::string& name);

  ~CachingComponentBase() override;

  const std::string& Name() const;
  std::shared_ptr<const T> Get() const;

  // Don't use this function, use Get() instead
  [[deprecated]] std::shared_ptr<T> GetLegacyToBeRemoved() const;

 protected:
  void Set(std::shared_ptr<const T> value_ptr);
  void Set(T&& value);

  template <typename... Args>
  void Emplace(Args&&... args);

  void Clear();

  formats::json::Value ExtendStatistics(
      const utils::statistics::StatisticsRequest& /*request*/);

  void OnConfigUpdate(const std::shared_ptr<const taxi_config::Config>& cfg);

 private:
  utils::statistics::Entry statistics_holder_;
  utils::SwappingSmart<const T> cache_;
  utils::AsyncEventSubscriberScope config_subscription_;
  server::CacheInvalidatorHolder cache_invalidator_holder_;
  const std::string name_;
};

template <typename T>
CachingComponentBase<T>::CachingComponentBase(const ComponentConfig& config,
                                              const ComponentContext& context,
                                              const std::string& name)
    : LoggableComponentBase(config, context),
      utils::AsyncEventChannel<const std::shared_ptr<const T>&>(name),
      CacheUpdateTrait(cache::CacheConfig(config), name),
      cache_invalidator_holder_(*this, context),
      name_(name) {
  auto& storage =
      context.FindComponent<components::StatisticsStorage>().GetStorage();
  statistics_holder_ = storage.RegisterExtender(
      "cache." + name_, std::bind(&CachingComponentBase<T>::ExtendStatistics,
                                  this, std::placeholders::_1));

  if (config.ParseBool("config-settings", true) &&
      cache::CacheConfigSet::IsConfigEnabled()) {
    auto& taxi_config = context.FindComponent<components::TaxiConfig>();
    OnConfigUpdate(taxi_config.Get());
    config_subscription_ = taxi_config.AddListener(
        this, "cache_" + name, &CachingComponentBase<T>::OnConfigUpdate);
  }
}

template <typename T>
CachingComponentBase<T>::~CachingComponentBase() {
  statistics_holder_.Unregister();
  config_subscription_.Unsubscribe();
}

template <typename T>
const std::string& CachingComponentBase<T>::Name() const {
  return name_;
}

template <typename T>
std::shared_ptr<const T> CachingComponentBase<T>::Get() const {
  return cache_.Get();
}

template <typename T>
std::shared_ptr<T> CachingComponentBase<T>::GetLegacyToBeRemoved() const {
  return std::const_pointer_cast<T>(cache_.Get());
}

template <typename T>
void CachingComponentBase<T>::Set(std::shared_ptr<const T> value_ptr) {
  cache_.Set(value_ptr);
  this->SendEvent(Get());
}

template <typename T>
void CachingComponentBase<T>::Set(T&& value) {
  Emplace(std::move(value));
}

template <typename T>
template <typename... Args>
void CachingComponentBase<T>::Emplace(Args&&... args) {
  Set(std::make_shared<T>(std::forward<Args>(args)...));
}

template <typename T>
void CachingComponentBase<T>::Clear() {
  cache_.Clear();
}

template <typename T>
formats::json::Value CachingComponentBase<T>::ExtendStatistics(
    const utils::statistics::StatisticsRequest& /*request*/) {
  /* Make copy to be able to make a more consistent combined statistics */
  const auto full = GetStatistics().full_update;
  const auto incremental = GetStatistics().incremental_update;
  const auto any = cache::CombineStatistics(full, incremental);

  formats::json::ValueBuilder builder;
  utils::statistics::SolomonLabelValue(builder, "cache_name");
  builder[cache::kStatisticsNameFull] = cache::StatisticsToJson(full);
  builder[cache::kStatisticsNameIncremental] =
      cache::StatisticsToJson(incremental);
  builder[cache::kStatisticsNameAny] = cache::StatisticsToJson(any);

  builder[cache::kStatisticsNameCurrentDocumentsCount] =
      GetStatistics().documents_current_count.load();

  return builder.ExtractValue();
}

template <typename T>
void CachingComponentBase<T>::OnConfigUpdate(
    const std::shared_ptr<const taxi_config::Config>& cfg) {
  SetConfig(cfg->Get<cache::CacheConfigSet>().GetConfig(name_));
}

}  // namespace components
