#pragma once

/// @file userver/dynamic_config/source.hpp
/// @brief @copybrief dynamic_config::Source

#include <string_view>
#include <utility>

#include <userver/concurrent/async_event_source.hpp>
#include <userver/dynamic_config/snapshot.hpp>

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
  using EventSource = concurrent::AsyncEventSource<const Snapshot&>;

  /// For internal use only. Obtain using components::DynamicConfig or
  /// dynamic_config::StorageMock instead.
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

  /// Subscribes to dynamic-config updates using a member function, named
  /// `OnConfigUpdate` by convention. Also immediately invokes the function with
  /// the current config snapshot.
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

  EventSource& GetEventChannel();

 private:
  concurrent::AsyncEventSubscriberScope DoUpdateAndListen(
      concurrent::FunctionId id, std::string_view name,
      EventSource::Function&& func);

  impl::StorageData* storage_;
};

}  // namespace dynamic_config

USERVER_NAMESPACE_END
