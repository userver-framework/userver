#pragma once

/// @file userver/cache/caching_component_base.hpp
/// @brief @copybrief components::CachingComponentBase

#include <memory>
#include <string>
#include <utility>

#include <fmt/format.h>

#include <userver/cache/cache_update_trait.hpp>
#include <userver/cache/exceptions.hpp>
#include <userver/compiler/demangle.hpp>
#include <userver/components/component_fwd.hpp>
#include <userver/components/loggable_component_base.hpp>
#include <userver/concurrent/async_event_channel.hpp>
#include <userver/dump/helpers.hpp>
#include <userver/dump/meta.hpp>
#include <userver/dump/operations.hpp>
#include <userver/engine/async.hpp>
#include <userver/rcu/rcu.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/impl/wait_token_storage.hpp>
#include <userver/utils/meta.hpp>
#include <userver/utils/shared_readable_ptr.hpp>
#include <userver/yaml_config/schema.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

// clang-format off

/// @ingroup userver_components userver_base_classes
///
/// @brief Base class for caching components
///
/// Provides facilities for creating periodically updated caches.
/// You need to override cache::CacheUpdateTrait::Update
/// then call cache::CacheUpdateTrait::StartPeriodicUpdates after setup
/// and cache::CacheUpdateTrait::StopPeriodicUpdates before teardown.
/// You can also override cache::CachingComponentBase::PreAssignCheck and set
/// has-pre-assign-check: true in the static config to enable check.
///
/// Caching components must be configured in service config (see options below)
/// and may be reconfigured dynamically via components::DynamicConfig.
///
/// @ref md_en_userver_caches provide a more detailed introduction.
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
/// update-jitter | max. amount of time by which interval may be adjusted for requests dispersal | update_interval / 10
/// full-update-interval | interval between full updates | --
/// updates-enabled | if false, cache updates are disabled (except for the first one if !first-update-fail-ok) | true
/// first-update-fail-ok | whether first update failure is non-fatal | false
/// task-processor | the name of the TaskProcessor for running DoWork | main-task-processor
/// config-settings | enables dynamic reconfiguration with CacheConfigSet | true
/// exception-interval | Used instead of `update-interval` in case of exception | update_interval
/// additional-cleanup-interval | how often to run background RCU garbage collector | 10 seconds
/// is-strong-period | whether to include Update execution time in update-interval | false
/// testsuite-force-periodic-update | override testsuite-periodic-update-enabled in TestsuiteSupport component config | --
/// failed-updates-before-expiration | the number of consecutive failed updates for data expiration | --
/// has-pre-assign-check | enables the check before changing the value in the cache, by default it is the check that the new value is not empty | false
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
///
/// @see `dump::Dumper` for more info on persistent cache dumps and
/// corresponding config options.
///
/// @see @ref md_en_userver_caches. pytest_userver.client.Client.invalidate_caches()
/// for a function to force cache update from testsuite.

// clang-format on

template <typename T>
// NOLINTNEXTLINE(fuchsia-multiple-inheritance)
class CachingComponentBase : public LoggableComponentBase,
                             protected cache::CacheUpdateTrait {
 public:
  CachingComponentBase(const ComponentConfig& config, const ComponentContext&);
  ~CachingComponentBase() override;

  using cache::CacheUpdateTrait::Name;

  using DataType = T;

  /// @return cache contents. May be nullptr if and only if MayReturnNull()
  /// returns true.
  utils::SharedReadablePtr<T> Get() const;

  /// @return cache contents. May be nullptr regardless of MayReturnNull().
  utils::SharedReadablePtr<T> GetUnsafe() const;

  /// Subscribes to cache updates using a member function. Also immediately
  /// invokes the function with the current cache contents.
  template <class Class>
  concurrent::AsyncEventSubscriberScope UpdateAndListen(
      Class* obj, std::string name,
      void (Class::*func)(const std::shared_ptr<const T>&));

  concurrent::AsyncEventChannel<const std::shared_ptr<const T>&>&
  GetEventChannel();

  static yaml_config::Schema GetStaticConfigSchema();

 protected:
  void Set(std::unique_ptr<const T> value_ptr);
  void Set(T&& value);

  template <typename... Args>
  void Emplace(Args&&... args);

  void Clear();

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
  void OnAllComponentsLoaded() final;

  void Cleanup() final;

  void MarkAsExpired() final;

  void GetAndWrite(dump::Writer& writer) const final;
  void ReadAndSet(dump::Reader& reader) final;

  /// @brief If the option has-pre-assign-check is set true in static config,
  /// this function is called before assigning the new value to the cache
  /// @note old_value_ptr and new_value_ptr can be nullptr.
  virtual void PreAssignCheck(const T* old_value_ptr,
                              const T* new_value_ptr) const;

  rcu::Variable<std::shared_ptr<const T>> cache_;
  concurrent::AsyncEventChannel<const std::shared_ptr<const T>&> event_channel_;
  utils::impl::WaitTokenStorage wait_token_storage_;
};

template <typename T>
CachingComponentBase<T>::CachingComponentBase(const ComponentConfig& config,
                                              const ComponentContext& context)
    : LoggableComponentBase(config, context),
      cache::CacheUpdateTrait(config, context),
      event_channel_(components::GetCurrentComponentName(config),
                     [this](auto& function) {
                       const auto ptr = cache_.ReadCopy();
                       if (ptr) function(ptr);
                     }) {
  const auto initial_config = GetConfig();
}

template <typename T>
CachingComponentBase<T>::~CachingComponentBase() {
  // Avoid a deadlock in WaitForAllTokens
  cache_.Assign(nullptr);
  // We must wait for destruction of all instances of T to finish, otherwise
  // it's UB if T's destructor accesses dependent components
  wait_token_storage_.WaitForAllTokens();
}

template <typename T>
utils::SharedReadablePtr<T> CachingComponentBase<T>::Get() const {
  auto ptr = GetUnsafe();
  if (!ptr && !MayReturnNull()) {
    throw cache::EmptyCacheError(Name());
  }
  return ptr;
}

template <typename T>
template <typename Class>
concurrent::AsyncEventSubscriberScope CachingComponentBase<T>::UpdateAndListen(
    Class* obj, std::string name,
    void (Class::*func)(const std::shared_ptr<const T>&)) {
  return event_channel_.DoUpdateAndListen(obj, std::move(name), func, [&] {
    auto ptr = Get();  // TODO: extra ref
    (obj->*func)(ptr);
  });
}

template <typename T>
concurrent::AsyncEventChannel<const std::shared_ptr<const T>&>&
CachingComponentBase<T>::GetEventChannel() {
  return event_channel_;
}

template <typename T>
utils::SharedReadablePtr<T> CachingComponentBase<T>::GetUnsafe() const {
  return utils::SharedReadablePtr<T>(cache_.ReadCopy());
}

template <typename T>
void CachingComponentBase<T>::Set(std::unique_ptr<const T> value_ptr) {
  auto deleter = [token = wait_token_storage_.GetToken(),
                  &cache_task_processor =
                      GetCacheTaskProcessor()](const T* raw_ptr) mutable {
    std::unique_ptr<const T> ptr{raw_ptr};

    // Kill garbage asynchronously as T::~T() might be very slow
    engine::CriticalAsyncNoSpan(cache_task_processor, [ptr = std::move(ptr),
                                                       token = std::move(
                                                           token)]() mutable {
      // Make sure *ptr is deleted before token is destroyed
      ptr.reset();
    }).Detach();
  };

  const std::shared_ptr<const T> new_value(value_ptr.release(),
                                           std::move(deleter));

  if (HasPreAssignCheck()) {
    auto old_value = cache_.Read();
    PreAssignCheck(old_value->get(), new_value.get());
  }

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

template <typename T>
void CachingComponentBase<T>::MarkAsExpired() {
  Set(std::unique_ptr<const T>{});
}

namespace impl {

yaml_config::Schema GetCachingComponentBaseSchema();

}

template <typename T>
yaml_config::Schema CachingComponentBase<T>::GetStaticConfigSchema() {
  return impl::GetCachingComponentBaseSchema();
}

template <typename T>
void CachingComponentBase<T>::PreAssignCheck(
    const T*, [[maybe_unused]] const T* new_value_ptr) const {
  UINVARIANT(
      meta::kIsSizable<T>,
      fmt::format("{} type does not support std::size(), add implementation of "
                  "the method size() for this type or "
                  "override cache::CachingComponentBase::PreAssignCheck.",
                  compiler::GetTypeName<T>()));

  if constexpr (meta::kIsSizable<T>) {
    if (!new_value_ptr || std::size(*new_value_ptr) == 0) {
      throw cache::EmptyDataError(Name());
    }
  }
}

}  // namespace components

USERVER_NAMESPACE_END
