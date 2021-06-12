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
  KeyValue(Key, VariableOfKey<Key> value)
      : id_(impl::kConfigId<Key>), value_(std::move(value)) {}

  /// Parses the value from `formats::json::Value`
  template <typename Key, typename Json, typename = impl::IsJson<Json>>
  KeyValue(Key, const Json& value)
      : id_(impl::kConfigId<Key>),
        value_(value.template As<VariableOfKey<Key>>()) {}

  /// Used by `MockStorage` internally
  void Store(std::vector<std::any>& configs) const { configs[id_] = value_; }

 private:
  impl::ConfigId id_;
  std::any value_;
};

/// @brief Backing storage for `taxi_config::Source` in tests and benchmarks
/// @warning Make sure that `StorageMock` outlives all the acquired pointers!
/// @snippet core/src/taxi_config/config_test.cpp  Sample StorageMock usage
/// @see taxi_config::GetDefaultSource
/// @see taxi_config::MakeDefaultStorage
class StorageMock final {
 public:
  /// Create an empty `StorageMock`
  StorageMock();

  /// Only store `config_variables` in the `Config`.
  /// Use as: `StorageMock{{kVariableKey1, value1}, {kVariableKey2, value2}}`
  StorageMock(std::initializer_list<KeyValue> config_variables);

  /// @brief Store `overrides` in the `Config`, then parse all the remaining
  /// variables from `defaults`
  /// @see taxi_config::MakeDefaultStorage
  StorageMock(const DocsMap& defaults, const std::vector<KeyValue>& overrides);

  StorageMock(StorageMock&&) noexcept = default;
  StorageMock& operator=(StorageMock&&) noexcept = default;

  /// Update some config variables
  void Patch(const std::vector<KeyValue>& overrides);

  taxi_config::Source GetSource() const&;

  // Store the StorageMock in a variable before using
  taxi_config::Source GetSource() && = delete;

  [[deprecated("Use MakeDefaultStorage instead")]] StorageMock(
      const DocsMap& docs_map);

  [[deprecated("Use GetSource instead")]] Source operator*() const;
  [[deprecated("Use GetSource instead")]] impl::SourceHolder operator->() const;

 private:
  std::unique_ptr<impl::Storage> storage_;
};

}  // namespace taxi_config
