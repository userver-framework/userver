#pragma once

#include <memory>
#include <type_traits>
#include <utility>

#include <rcu/rcu.hpp>
#include <taxi_config/config.hpp>

namespace components {
class ComponentContext;
}

namespace taxi_config {

namespace impl {

using Storage = rcu::Variable<Config>;

const Storage& FindStorage(const components::ComponentContext& context);

}  // namespace impl

/// Owns a snapshot of config. You may use operator* or operator[]
/// to access the config.
class SnapshotPtr final {
 public:
  SnapshotPtr(SnapshotPtr&&) noexcept = default;
  SnapshotPtr& operator=(SnapshotPtr&&) noexcept = default;

  const Config& operator*() const&;
  const Config* operator->() const&;

  // Store the SnapshotPtr in a variable before using
  const Config& operator*() && = delete;
  const Config* operator->() && = delete;

  template <typename Key>
  const VariableOfKey<Key>& operator[](Key key) const& {
    return (*container_)[key];
  }

  template <typename Key>
  const VariableOfKey<Key>& operator[](Key) && {
    static_assert(!sizeof(Key), "keep the pointer before using, please");
  }

 private:
  explicit SnapshotPtr(const impl::Storage& storage)
      : container_(storage.Read()) {}

  // for the constructor
  friend class Source;

  // for the constructor
  template <typename Key>
  friend class VariableSnapshotPtr;

  rcu::ReadablePtr<Config> container_;
};

/// Owns a snapshot of a config variable. You may use operator* or operator->
/// to access the config variable.
///
/// `VariableSnapshotPtr` in only intended to be used locally. Don't store it
/// as a class member or pass it between functions. Use `SnapshotPtr` for that
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

  explicit VariableSnapshotPtr(const impl::Storage& storage, Key key)
      : snapshot_(storage), variable_(snapshot_[key]) {}

  // for the constructor
  friend class Source;

  SnapshotPtr snapshot_;
  const VariableOfKey<Key>& variable_;
};

/// A helper for easy config fetching in components. After construction, Source
/// can be copied around and passed to clients or child helper classes.
class Source final {
 public:
  explicit Source(const components::ComponentContext& context);

  // trivially copyable
  Source(const Source&) = default;
  Source& operator=(const Source&) = default;

  SnapshotPtr GetSnapshot() const;

  template <typename Key>
  VariableSnapshotPtr<Key> GetSnapshot(Key key) const {
    return VariableSnapshotPtr{*storage_, key};
  }

  template <typename Key>
  VariableOfKey<Key> GetCopy(Key key) const {
    const auto snapshot = GetSnapshot();
    return snapshot[key];
  }

 private:
  explicit Source(const impl::Storage& storage);

  // for the constructor
  friend class StorageMock;

  const impl::Storage* storage_;
};

}  // namespace taxi_config
