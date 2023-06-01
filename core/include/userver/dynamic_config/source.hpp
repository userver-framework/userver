#pragma once

/// @file userver/dynamic_config/source.hpp
/// @brief @copybrief dynamic_config::Source

#include <string_view>
#include <tuple>
#include <utility>

#include <userver/concurrent/async_event_source.hpp>
#include <userver/dynamic_config/snapshot.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace dynamic_config {

/// Owns a snapshot of a config variable. You may use operator* or operator->
/// to access the config variable.
///
/// `VariableSnapshotPtr` in only intended to be used locally. Don't store it
/// as a class member or pass it between functions. Use `Snapshot` for that
/// purpose.
template <typename Key>
class VariableSnapshotPtr final {
 public:
  VariableSnapshotPtr(VariableSnapshotPtr&&) = delete;
  VariableSnapshotPtr& operator=(VariableSnapshotPtr&&) = delete;

  const VariableOfKey<Key>& operator*() const& { return variable_; }
  const VariableOfKey<Key>& operator*() && { ReportMisuse(); }

  const VariableOfKey<Key>* operator->() const& { return &variable_; }
  const VariableOfKey<Key>* operator->() && { ReportMisuse(); }

 private:
  [[noreturn]] static void ReportMisuse() {
    static_assert(!sizeof(Key), "keep the pointer before using, please");
  }

  explicit VariableSnapshotPtr(Snapshot&& snapshot, Key key)
      : snapshot_(std::move(snapshot)), variable_(snapshot_[key]) {}

  // for the constructor
  friend class Source;

  Snapshot snapshot_;
  const VariableOfKey<Key>& variable_;
};

/// @brief Helper class for subscribing to dynamic-config updates with a custom
/// callback.
///
/// Stores information about the last update that occurred.
///
/// @param previous `dynamic_config::Snapshot` of the previous config or
/// `std::nullopt` if this update event is the first for the subscriber.
struct Diff final {
  std::optional<Snapshot> previous;
  Snapshot current;
};

// clang-format off

/// @ingroup userver_clients
///
/// @brief A client for easy dynamic config fetching in components.
///
/// After construction, dynamic_config::Source
/// can be copied around and passed to clients or child helper classes.
///
/// Usually retrieved from components::DynamicConfig component.
///
/// Typical usage:
/// @snippet components/component_sample_test.cpp  Sample user component runtime config source

// clang-format on
class Source final {
 public:
  using SnapshotEventSource = concurrent::AsyncEventSource<const Snapshot&>;
  using DiffEventSource = concurrent::AsyncEventSource<const Diff&>;

  /// For internal use only. Obtain using components::DynamicConfig or
  /// dynamic_config::StorageMock instead.
  explicit Source(impl::StorageData& storage);

  // trivially copyable
  Source(const Source&) = default;
  Source(Source&&) = default;
  Source& operator=(const Source&) = default;
  Source& operator=(Source&&) = default;

  Snapshot GetSnapshot() const;

  template <typename Key>
  VariableSnapshotPtr<Key> GetSnapshot(Key key) const {
    return VariableSnapshotPtr{GetSnapshot(), key};
  }

  template <typename Key>
  VariableOfKey<Key> GetCopy(Key key) const {
    const auto snapshot = GetSnapshot();
    return snapshot[key];
  }

  /// Subscribes to dynamic-config updates using a member function. Also
  /// immediately invokes the function with the current config snapshot (this
  /// invocation will be executed synchronously).
  ///
  /// @note Callbacks occur in full accordance with
  /// `components::DynamicConfigClientUpdater` options.
  ///
  /// @param obj the subscriber, which is the owner of the listener method, and
  /// is also used as the unique identifier of the subscription
  /// @param name the name of the subscriber, for diagnostic purposes
  /// @param func the listener method, named `OnConfigUpdate` by convention.
  /// @returns a `concurrent::AsyncEventSubscriberScope` controlling the
  /// subscription, which should be stored as a member in the subscriber;
  /// `Unsubscribe` should be called explicitly
  ///
  /// @see based on concurrent::AsyncEventSource engine
  template <typename Class>
  concurrent::AsyncEventSubscriberScope UpdateAndListen(
      Class* obj, std::string_view name,
      void (Class::*func)(const dynamic_config::Snapshot& config)) {
    return DoUpdateAndListen(
        concurrent::FunctionId(obj), name,
        [obj, func](const dynamic_config::Snapshot& config) {
          (obj->*func)(config);
        });
  }

  // clang-format off

  /// @brief Subscribes to dynamic-config updates with information about the
  /// current and previous states.
  ///
  /// Subscribes to dynamic-config updates using a member function, named
  /// `OnConfigUpdate` by convention. Also constructs `dynamic_config::Diff`
  /// object using `std::nullopt` and current config snapshot, then immediately
  /// invokes the function with it (this invocation will be executed
  /// synchronously).
  ///
  /// @note Callbacks occur in full accordance with
  /// `components::DynamicConfigClientUpdater` options.
  ///
  /// @warning In debug mode the last notification for any subscriber will be
  /// called with `std::nullopt` and current config snapshot.
  ///
  /// Example usage:
  /// @snippet dynamic_config/config_test.cpp Custom subscription for dynamic config update
  ///
  /// @param obj the subscriber, which is the owner of the listener method, and
  /// is also used as the unique identifier of the subscription
  /// @param name the name of the subscriber, for diagnostic purposes
  /// @param func the listener method, named `OnConfigUpdate` by convention.
  /// @returns a `concurrent::AsyncEventSubscriberScope` controlling the
  /// subscription, which should be stored as a member in the subscriber;
  /// `Unsubscribe` should be called explicitly
  ///
  /// @see based on concurrent::AsyncEventSource engine
  ///
  /// @see dynamic_config::Diff

  // clang-format on
  template <typename Class>
  concurrent::AsyncEventSubscriberScope UpdateAndListen(
      Class* obj, std::string_view name,
      void (Class::*func)(const dynamic_config::Diff& diff)) {
    return DoUpdateAndListen(
        concurrent::FunctionId(obj), name,
        [obj, func](const dynamic_config::Diff& diff) { (obj->*func)(diff); });
  }

  /// @brief Subscribes to updates of a subset of all configs.
  ///
  /// Subscribes to dynamic-config updates using a member function, named
  /// `OnConfigUpdate` by convention. The function will be invoked if at least
  /// one of the configs has been changed since the previous invocation. So at
  /// the first time immediately invokes the function with the current config
  /// snapshot (this invocation will be executed synchronously).
  ///
  /// @note Сallbacks occur only if one of the passed config is changed. This is
  /// true under any components::DynamicConfigClientUpdater options.
  ///
  /// @warning To use this function, configs must have the `operator==`.
  ///
  /// @param obj the subscriber, which is the owner of the listener method, and
  /// is also used as the unique identifier of the subscription
  /// @param name the name of the subscriber, for diagnostic purposes
  /// @param func the listener method, named `OnConfigUpdate` by convention.
  /// @param keys config objects, specializations of `dynamic_config::Key`.
  /// @returns a `concurrent::AsyncEventSubscriberScope` controlling the
  /// subscription, which should be stored as a member in the subscriber;
  /// `Unsubscribe` should be called explicitly
  ///
  /// @see based on concurrent::AsyncEventSource engine
  template <typename Class, typename... Keys>
  concurrent::AsyncEventSubscriberScope UpdateAndListen(
      Class* obj, std::string_view name,
      void (Class::*func)(const dynamic_config::Snapshot& config),
      Keys... keys) {
    auto wrapper = [obj, func,
                    keys = std::make_tuple(keys...)](const Diff& diff) {
      const auto args = std::tuple_cat(std::tie(diff), keys);
      if (!std::apply(HasChanged<Keys...>, args)) return;
      (obj->*func)(diff.current);
    };
    return DoUpdateAndListen(concurrent::FunctionId(obj), name,
                             std::move(wrapper));
  }

  SnapshotEventSource& GetEventChannel();

 private:
  template <typename... Keys>
  static bool HasChanged(const Diff& diff, Keys... keys) {
    if (!diff.previous) return true;

    const auto& previous = *diff.previous;
    const auto& current = diff.current;

    UASSERT(!current.GetData().IsEmpty());
    UASSERT(!previous.GetData().IsEmpty());

    const bool is_equal = (true && ... && (previous[keys] == current[keys]));
    return !is_equal;
  }

  concurrent::AsyncEventSubscriberScope DoUpdateAndListen(
      concurrent::FunctionId id, std::string_view name,
      SnapshotEventSource::Function&& func);

  concurrent::AsyncEventSubscriberScope DoUpdateAndListen(
      concurrent::FunctionId id, std::string_view name,
      DiffEventSource::Function&& func);

  impl::StorageData* storage_;
};

}  // namespace dynamic_config

USERVER_NAMESPACE_END
