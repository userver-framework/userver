#pragma once

/// @file cache/caching_component_base.hpp
/// @brief @copybrief components::CachingComponentBase

#include <atomic>
#include <memory>
#include <mutex>

#include <components/statistics_storage.hpp>
#include <rcu/rcu.hpp>
#include <taxi_config/storage/component.hpp>
#include <testsuite/cache_control.hpp>
#include <testsuite/testsuite_support.hpp>
#include <utils/async_event_channel.hpp>

#include <components/component_config.hpp>
#include <components/loggable_component_base.hpp>

#include <cache/cache_config.hpp>
#include <cache/cache_update_trait.hpp>
#include <dump/factory.hpp>
#include <dump/meta.hpp>
#include <dump/operations.hpp>

namespace components {

// clang-format off

/// @ingroup userver_components
///
/// @brief Base class for caching components
///
/// Provides facilities for creating periodically updated caches.
/// You need to override CacheUpdateTrait::Update
/// then call CacheUpdateTrait::StartPeriodicUpdates after setup
/// and CacheUpdateTrait::StopPeriodicUpdates before teardown.
///
/// Caching components must be configured in service config (see options below)
/// and may be reconfigured dynamically via components::TaxiConfig.
///
/// ## Dynamic config
/// * @ref USERVER_CACHES
/// * @ref USERVER_DUMPS
///
/// ## Static options:
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// update-types | specifies whether incremental and/or full updates will be used | see below
/// update-interval | (*required*) interval between Update invocations | --
/// update-jitter | max. amount of time by which interval may be adjusted for requests desynchronization | update_interval / 10
/// full-update-interval | interval between full updates | --
/// first-update-fail-ok | whether first update failure is non-fatal | false
/// config-settings | enables dynamic reconfiguration with CacheConfigSet | true
/// additional-cleanup-interval | how often to run background RCU garbase collector | 10 seconds
/// testsuite-force-periodic-update | override testsuite-periodic-update-enabled in TestsuiteSupport component config | --
///
/// ### Update types
///  * `full-and-incremental`: both `update-interval` and `full-update-interval`
///    must be specified. Updates with UpdateType::kIncremental will be triggered
///    each `update-interval` (adjusted by jitter) unless `full-update-interval`
///    has passed and UpdateType::kFull is triggered.
///  * `only-full`: only `update-interval` must be specified. UpdateType::kFull
///    will be triggered each `update-interval` (adjusted by jitter).
///  * `only-incremental`: only `update-interval` must be specified. UpdateType::kFull is triggered
///    on the first update, afterwards UpdateType::kIncremental will be triggered
///    each `update-interval` (adjusted by jitter).
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
// NOLINTNEXTLINE(fuchsia-multiple-inheritance)
class CachingComponentBase : public LoggableComponentBase,
                             protected cache::CacheUpdateTrait {
 public:
  CachingComponentBase(const ComponentConfig& config, const ComponentContext&);

  using cache::CacheUpdateTrait::Name;

  using DataType = T;

  /// @return cache contents. May be nullptr if and only if MayReturnNull()
  /// returns true.
  std::shared_ptr<const T> Get() const;

  /// @return cache contents. May be nullptr regardless of MayReturnNull().
  std::shared_ptr<const T> GetUnsafe() const;

  template <class Class>
  ::concurrent::AsyncEventSubscriberScope UpdateAndListen(
      Class* obj, std::string name,
      void (Class::*func)(const std::shared_ptr<const T>&));

  concurrent::AsyncEventChannel<const std::shared_ptr<const T>&>&
  GetEventChannel();

 protected:
  void Set(std::unique_ptr<const T> value_ptr);
  void Set(T&& value);

  template <typename... Args>
  void Emplace(Args&&... args);

  void Clear();

  void OnConfigUpdate(const std::shared_ptr<const taxi_config::Config>& cfg);

  /// Whether Get() is expected to return nullptr.
  /// If MayReturnNull() returns false, Get() throws an exception instead of
  /// returning nullptr.
  virtual bool MayReturnNull() const;

  /// @{
  /// Override to use custom serialization for cache dumps
  virtual void WriteContents(dump::Writer& writer, const T& contents) const;

  virtual std::unique_ptr<const T> ReadContents(dump::Reader& reader) const;
  /// @}

 private:
  void OnAllComponentsLoaded() override;

  void Cleanup() final;

  void GetAndWrite(dump::Writer& writer) const final;
  void ReadAndSet(dump::Reader& reader) final;

  concurrent::AsyncEventChannel<const std::shared_ptr<const T>&> event_channel_;
  utils::statistics::Entry statistics_holder_;
  rcu::Variable<std::shared_ptr<const T>> cache_;
  concurrent::AsyncEventSubscriberScope config_subscription_;
};

template <typename T>
CachingComponentBase<T>::CachingComponentBase(const ComponentConfig& config,
                                              const ComponentContext& context)
    : LoggableComponentBase(config, context),
      cache::CacheUpdateTrait(config, context),
      event_channel_(config.Name()) {
  const auto initial_config = GetConfig();

  auto& storage =
      context.FindComponent<components::StatisticsStorage>().GetStorage();
  statistics_holder_ = storage.RegisterExtender(
      "cache." + Name(), [this](auto&) { return ExtendStatistics(); });

  if (initial_config->config_updates_enabled) {
    auto& taxi_config = context.FindComponent<components::TaxiConfig>();
    config_subscription_ = taxi_config.UpdateAndListen(
        this, "cache_" + Name(), &CachingComponentBase<T>::OnConfigUpdate);
  }
}

template <typename T>
std::shared_ptr<const T> CachingComponentBase<T>::Get() const {
  auto ptr = GetUnsafe();
  if (!ptr && !MayReturnNull()) {
    throw cache::EmptyCacheError(Name());
  }
  return ptr;
}

template <typename T>
template <typename Class>
::concurrent::AsyncEventSubscriberScope
CachingComponentBase<T>::UpdateAndListen(
    Class* obj, std::string name,
    void (Class::*func)(const std::shared_ptr<const T>&)) {
  return event_channel_.DoUpdateAndListen(obj, std::move(name), func,
                                          [&] { (obj->*func)(Get()); });
}

template <typename T>
concurrent::AsyncEventChannel<const std::shared_ptr<const T>&>&
CachingComponentBase<T>::GetEventChannel() {
  return event_channel_;
}

template <typename T>
std::shared_ptr<const T> CachingComponentBase<T>::GetUnsafe() const {
  return cache_.ReadCopy();
}

template <typename T>
void CachingComponentBase<T>::Set(std::unique_ptr<const T> value_ptr) {
  constexpr auto deleter = [](const T* raw_ptr) {
    std::unique_ptr<const T> ptr{raw_ptr};

    engine::impl::CriticalAsync([ptr = std::move(ptr)]() mutable {
      // Kill garbage asynchronously as T::~T() might be very slow
      ptr.reset();
    }).Detach();
  };

  const std::shared_ptr<const T> new_value(value_ptr.release(), deleter);
  cache_.Assign(new_value);
  event_channel_.SendEvent(new_value);
  OnCacheModified();
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
void CachingComponentBase<T>::OnConfigUpdate(
    const std::shared_ptr<const taxi_config::Config>& cfg) {
  SetConfigPatch(cfg->Get<cache::CacheConfigSet>().GetConfig(Name()));
  SetConfigPatch(cfg->Get<dump::ConfigSet>().GetConfig(Name()));
}

template <typename T>
bool CachingComponentBase<T>::MayReturnNull() const {
  return false;
}

template <typename T>
void CachingComponentBase<T>::GetAndWrite(dump::Writer& writer) const {
  const auto contents = GetUnsafe();
  if (!contents) throw cache::EmptyCacheError(Name());
  WriteContents(writer, *contents);
}

template <typename T>
void CachingComponentBase<T>::ReadAndSet(dump::Reader& reader) {
  Set(ReadContents(reader));
}

template <typename T>
void CachingComponentBase<T>::WriteContents(dump::Writer& writer,
                                            const T& contents) const {
  if constexpr (dump::kIsDumpable<T>) {
    writer.Write(contents);
  } else {
    dump::ThrowDumpUnimplemented(Name());
  }
}

template <typename T>
std::unique_ptr<const T> CachingComponentBase<T>::ReadContents(
    dump::Reader& reader) const {
  if constexpr (dump::kIsDumpable<T>) {
    // To avoid an extra move and avoid including common_containers.hpp
    return std::unique_ptr<const T>{new T(reader.Read<T>())};
  } else {
    dump::ThrowDumpUnimplemented(Name());
  }
}

template <typename T>
void CachingComponentBase<T>::OnAllComponentsLoaded() {
  AssertPeriodicUpdateStarted();
}

template <typename T>
void CachingComponentBase<T>::Cleanup() {
  cache_.Cleanup();
}

}  // namespace components
