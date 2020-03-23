#pragma once

/// @file cache/caching_component_base.hpp
/// @brief @copybrief components::CachingComponentBase

#include <atomic>
#include <memory>
#include <mutex>

#include <cache/cache_statistics.hpp>
#include <components/statistics_storage.hpp>
#include <rcu/rcu.hpp>
#include <taxi_config/storage/component.hpp>
#include <testsuite/cache_control.hpp>
#include <testsuite/testsuite_support.hpp>
#include <utils/async_event_channel.hpp>
#include <utils/statistics/metadata.hpp>

#include <components/component_config.hpp>
#include <components/loggable_component_base.hpp>
#include "cache_config.hpp"
#include "cache_update_trait.hpp"

namespace components {

class EmptyCacheError : public std::runtime_error {
 public:
  EmptyCacheError(const std::string& cache_name)
      : std::runtime_error("Cache " + cache_name + " is empty") {}
};

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
/// first-update-fail-ok | whether first update failure is non-fatal | false
/// config-settings | enables dynamic reconfiguration with CacheConfigSet | true
/// testsuite-force-periodic-update | override testsuite-periodic-update-enabled in TestsuiteSupport component config | --
///
/// ### Update types
///  * `full-and-incremental`: both `update-interval` and `full-update-interval`
///    must be specified. Updates with UpdateType::kIncremental will be triggered
///    each `update-interval` (adjusted by jitter) unless `full-update-interval`
///    has passed and UpdateType::kFull is triggered.
///  * `only-full`: only `update-interval` must be specified. UpdateType::kFull
///    will be triggered each `update-interval` (adjusted by jitter).
///
/// ### testsuite-force-periodic-update
///  use it to enable periodic cache update for a component in testsuite environment
///  where testsuite-periodic-update-enabled from TestsuiteSupport config is false
///
/// By default, update types are guessed based on update intervals presence.
/// If both `update-interval` and `full-update-interval` are present,
/// `full-and-incremental` types is assumed. Otherwise `only-full` is used.

// clang-format on

template <typename T>
class CachingComponentBase
    : public LoggableComponentBase,
      public utils::AsyncEventChannel<const std::shared_ptr<const T>&>,
      protected cache::CacheUpdateTrait {
 public:
  CachingComponentBase(const ComponentConfig& config, const ComponentContext&,
                       const std::string& name);

  const std::string& Name() const;

  /// @return cache contents. May be nullptr if and only if MayReturnNull()
  /// returns true.
  std::shared_ptr<const T> Get() const;

  /// @return cache contents. May be nullptr regardless of MayReturnNull().
  std::shared_ptr<const T> GetUnsafe() const;

 protected:
  void Set(std::unique_ptr<const T> value_ptr);
  void Set(T&& value);

  template <typename... Args>
  void Emplace(Args&&... args);

  void Clear();

  formats::json::Value ExtendStatistics(
      const utils::statistics::StatisticsRequest& /*request*/);

  void OnConfigUpdate(const std::shared_ptr<const taxi_config::Config>& cfg);

  bool IsPeriodicUpdateEnabled() const override {
    return periodic_update_enabled_;
  }

  /// Whether Get() is expected to return nullptr.
  /// If MayReturnNull() returns false, Get() throws an exception instead of
  /// returning nullptr.
  virtual bool MayReturnNull() const;

 private:
  void OnAllComponentsLoaded() override;

  utils::statistics::Entry statistics_holder_;
  rcu::Variable<std::shared_ptr<const T>> cache_;
  utils::AsyncEventSubscriberScope config_subscription_;
  bool periodic_update_enabled_;
  const std::string name_;
};

template <typename T>
CachingComponentBase<T>::CachingComponentBase(const ComponentConfig& config,
                                              const ComponentContext& context,
                                              const std::string& name)
    : LoggableComponentBase(config, context),
      utils::AsyncEventChannel<const std::shared_ptr<const T>&>(name),
      cache::CacheUpdateTrait(
          cache::CacheConfig(config),
          context.FindComponent<components::TestsuiteSupport>()
              .GetCacheControl(),
          name),
      periodic_update_enabled_(context.FindComponent<TestsuiteSupport>()
                                   .GetCacheControl()
                                   .IsPeriodicUpdateEnabled(config, name)),
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
const std::string& CachingComponentBase<T>::Name() const {
  return name_;
}

template <typename T>
std::shared_ptr<const T> CachingComponentBase<T>::Get() const {
  auto ptr = GetUnsafe();
  if (!ptr && !MayReturnNull()) {
    UASSERT_MSG(ptr, "Cache returned nullptr, go and fix your cache");
    throw EmptyCacheError(Name());
  }
  return ptr;
}

template <typename T>
std::shared_ptr<const T> CachingComponentBase<T>::GetUnsafe() const {
  auto ptr = cache_.Read();
  return *ptr;
}

template <typename T>
void CachingComponentBase<T>::Set(std::unique_ptr<const T> value_ptr) {
  constexpr auto deleter = [](const T* raw_ptr) {
    std::unique_ptr<const T> ptr{raw_ptr};

    engine::impl::CriticalAsync([ptr = std::move(ptr)]() mutable {
      // Kill garbage asynchronously as T::~T() might be very slow
      ptr.reset();
    })
        .Detach();
  };

  std::shared_ptr<const T> new_value(value_ptr.release(), deleter);
  cache_.Assign(std::move(new_value));
  this->SendEvent(Get());
}

template <typename T>
void CachingComponentBase<T>::Set(T&& value) {
  Emplace(std::move(value));
}

template <typename T>
template <typename... Args>
void CachingComponentBase<T>::Emplace(Args&&... args) {
  Set(std::make_unique<T>(std::forward<Args>(args)...));
}

template <typename T>
void CachingComponentBase<T>::Clear() {
  cache_.Assign(std::make_unique<const T>());
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

template <typename T>
bool CachingComponentBase<T>::MayReturnNull() const {
  return false;
}

template <typename T>
void CachingComponentBase<T>::OnAllComponentsLoaded() {
  AssertPeriodicUpdateStarted();
}

}  // namespace components
