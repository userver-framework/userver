#pragma once

#include <userver/rcu/rcu.hpp>
#include <userver/taxi_config/snapshot_impl.hpp>
#include <userver/taxi_config/value.hpp>

namespace taxi_config {

// clang-format off
/// @brief A config key is a unique identifier for a config variable
/// @snippet core/src/components/logging_configurator.cpp  LoggingConfigurator config key
// clang-format on
template <auto Parser>
struct Key final {
  static auto Parse(const DocsMap& docs_map) { return Parser(docs_map); }
};

/// Get the type of a config variable, given its key
template <typename Key>
using VariableOfKey = decltype(Key::Parse(DocsMap{}));

// clang-format off
/// @brief The storage for a snapshot of configs
///
/// When a config update comes in via new `DocsMap`, configs of all
/// the registered types are constructed and stored in `Config`. After that
/// the `DocsMap` is dropped.
///
/// Config types are automatically registered if they are accessed with `Get`
/// somewhere in the program.
///
/// ## Usage example:
/// @snippet components/component_sample_test.cpp  Sample user component runtime config source
// clang-format on
class Snapshot final {
 public:
  Snapshot(const Snapshot&) = default;
  Snapshot& operator=(const Snapshot&) = default;

  Snapshot(Snapshot&&) noexcept = default;
  Snapshot& operator=(Snapshot&&) noexcept = default;

  /// Used to access individual configs in the type-safe config map
  template <typename Key>
  const VariableOfKey<Key>& operator[](Key key) const& {
    return (*container_)[key];
  }

  /// Used to access individual configs in the type-safe config map
  template <typename Key>
  const VariableOfKey<Key>& operator[](Key) && {
    static_assert(!sizeof(Key), "keep the Snapshot before using, please");
  }

  /// Deprecated, use `config[key]` instead
  template <typename T>
  const T& Get() const& {
    return (*this)[Key<impl::ParseByConstructor<T>>{}];
  }

  /// Deprecated, use `config[key]` instead
  template <typename T>
  const T& Get() && {
    static_assert(!sizeof(T), "keep the Snapshot before using, please");
  }

  const Snapshot& operator*() const;
  const Snapshot* operator->() const;

 private:
  explicit Snapshot(const impl::StorageData& storage);

  // for the constructor
  friend class Source;

  rcu::SharedReadablePtr<impl::SnapshotData> container_;
};

}  // namespace taxi_config
