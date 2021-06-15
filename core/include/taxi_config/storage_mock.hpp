#pragma once

#include <initializer_list>
#include <memory>
#include <vector>

#include <formats/json/value.hpp>
#include <taxi_config/config.hpp>
#include <taxi_config/config_ptr.hpp>
#include <taxi_config/value.hpp>

namespace taxi_config {
namespace impl {

template <typename T>
using IsJson = std::enable_if_t<std::is_same_v<T, formats::json::Value>>;

struct SourceHolder final {
  Source value;

  const Source* operator->() const { return &value; }
};

}  // namespace impl

/// A type-erased config key-value pair
class KeyValue final {
 public:
  /// Uses the provided value directly. It can also be constructed in-place:
  /// @code
  /// {kMyConfig, {"foo", 42}}
  /// @endcode
  template <typename Key>
  KeyValue(Key /*key*/, VariableOfKey<Key> value)
      : id_(impl::kConfigId<Key>), value_(std::move(value)) {}

  /// Parses the value from `formats::json::Value`
  template <typename Key, typename Json, typename = impl::IsJson<Json>>
  KeyValue(Key /*key*/, const Json& value)
      : id_(impl::kConfigId<Key>),
        value_(value.template As<VariableOfKey<Key>>()) {}

  /// For internal use only
  impl::ConfigId GetId() const { return id_; }

  /// For internal use only
  std::any GetValue() const { return value_; }

 private:
  impl::ConfigId id_;
  std::any value_;
};

/// @brief Backing storage for `taxi_config::Source` in tests and benchmarks
/// @warning Make sure that `StorageMock` outlives all the acquired pointers!
/// @snippet core/src/taxi_config/config_test.cpp  Sample StorageMock usage
/// @snippet core/src/taxi_config/config_test.cpp  StorageMock from JSON
/// @see taxi_config::GetDefaultSource
/// @see taxi_config::MakeDefaultStorage
class StorageMock final {
 public:
  /// Create an empty `StorageMock`
  StorageMock();

  /// Only store `config_variables` in the `Config`.
  /// Use as: `StorageMock{{kVariableKey1, value1}, {kVariableKey2, value2}}`
  StorageMock(std::initializer_list<KeyValue> config_variables);

  /// Only store `config_variables` in the `Config`
  explicit StorageMock(const std::vector<KeyValue>& config_variables);

  /// @brief Store `overrides` in the `Config`, then parse all the remaining
  /// variables from `defaults`
  /// @see taxi_config::MakeDefaultStorage
  StorageMock(const DocsMap& defaults, const std::vector<KeyValue>& overrides);

  StorageMock(StorageMock&&) noexcept = default;
  StorageMock& operator=(StorageMock&&) noexcept = default;

  /// Update some config variables
  void Extend(const std::vector<KeyValue>& overrides);

  taxi_config::Source GetSource() const&;
  taxi_config::SnapshotPtr GetSnapshot() const&;

  // Store the StorageMock in a variable before using
  taxi_config::Source GetSource() && = delete;
  taxi_config::SnapshotPtr GetSnapshot() && = delete;

  [[deprecated("Use MakeDefaultStorage instead")]] StorageMock(
      const DocsMap& docs_map);

  [[deprecated("Use GetSource instead")]] Source operator*() const;
  [[deprecated("Use GetSource instead")]] impl::SourceHolder operator->() const;

 private:
  std::unique_ptr<impl::Storage> storage_;
};

}  // namespace taxi_config
