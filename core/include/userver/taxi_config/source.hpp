#pragma once

#include <memory>
#include <type_traits>
#include <utility>

#include <userver/concurrent/async_event_channel.hpp>
#include <userver/rcu/rcu.hpp>
#include <userver/taxi_config/snapshot.hpp>

namespace taxi_config {

namespace impl {

struct StorageData final {
  rcu::Variable<SnapshotData> config;
  concurrent::AsyncEventChannel<const Snapshot&> channel{"taxi-config"};
};

}  // namespace impl

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

// clang-format off

/// @ingroup userver_clients
///
/// @brief A client for easy dynamic config fetching in components.
///
/// After construction, taxi_config::Source
/// can be copied around and passed to clients or child helper classes.
///
/// Usually retrieved from components::TaxiConfig component.
///
/// Typical usage:
/// @snippet components/component_sample_test.cpp  Sample user component runtime config source

// clang-format on
class Source final {
 public:
  /// For internal use only. Obtain using components::TaxiConfig or
  /// taxi_config::StorageMock instead.
  explicit Source(impl::StorageData& storage);

  // trivially copyable
  Source(const Source&) = default;
  Source& operator=(const Source&) = default;

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

  /// Subscribes to taxi-config updates using a member function, named
  /// `OnConfigUpdate` by convention. Also immediately invokes the function with
  /// the current config snapshot.
  template <typename Class>
  ::concurrent::AsyncEventSubscriberScope UpdateAndListen(
      Class* obj, std::string name,
      void (Class::*func)(const taxi_config::Snapshot& config)) {
    return GetEventChannel().DoUpdateAndListen(
        obj, std::move(name), func, [&] { (obj->*func)(GetSnapshot()); });
  }

  concurrent::AsyncEventChannel<const Snapshot&>& GetEventChannel();

 private:
  impl::StorageData* storage_;
};

}  // namespace taxi_config
