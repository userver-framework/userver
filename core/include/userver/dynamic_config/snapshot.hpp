#pragma once

/// @file userver/dynamic_config/snapshot.hpp
/// @brief @copybrief dynamic_config::Snapshot

#include <optional>
#include <type_traits>

#include <userver/dynamic_config/impl/snapshot.hpp>
#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace dynamic_config {

/// @brief A config key is a unique identifier for a config variable
/// @snippet core/src/components/logging_configurator.cpp  key
template <auto Parser>
struct Key final {
  static auto Parse(const DocsMap& docs_map) { return Parser(docs_map); }
};

/// Get the type of a config variable, given its key
template <typename Key>
using VariableOfKey = impl::VariableOfKey<Key>;

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
  Snapshot(const Snapshot&);
  Snapshot& operator=(const Snapshot&);

  Snapshot(Snapshot&&) noexcept;
  Snapshot& operator=(Snapshot&&) noexcept;

  ~Snapshot();

  /// Used to access individual configs in the type-safe config map
  template <typename Key>
  const VariableOfKey<Key>& operator[](Key key) const& {
    return GetData()[key];
  }

  /// Used to access individual configs in the type-safe config map
  template <typename Key>
  const VariableOfKey<Key>& operator[](Key) && {
    static_assert(!sizeof(Key), "keep the Snapshot before using, please");
  }

  /// Deprecated, use `config[key]` instead
  template <typename T>
  const T& Get() const& {
    return GetData()[Key<impl::ParseByConstructor<T>>{}];
  }

  /// Deprecated, use `config[key]` instead
  template <typename T>
  const T& Get() && {
    static_assert(!sizeof(T), "keep the Snapshot before using, please");
  }

 private:
  explicit Snapshot(const impl::StorageData& storage);

  const impl::SnapshotData& GetData() const;

  // for the constructor
  friend class Source;
  friend class impl::StorageData;

  struct Impl;
  utils::FastPimpl<Impl, 16, 8> impl_;
};

}  // namespace dynamic_config

USERVER_NAMESPACE_END
