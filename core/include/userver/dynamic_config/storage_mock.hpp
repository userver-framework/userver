#pragma once

/// @file userver/dynamic_config/storage_mock.hpp
/// @brief @copybrief dynamic_config::StorageMock

#include <initializer_list>
#include <memory>
#include <vector>

#include <userver/dynamic_config/snapshot.hpp>
#include <userver/dynamic_config/source.hpp>
#include <userver/dynamic_config/value.hpp>
#include <userver/formats/json/value.hpp>

USERVER_NAMESPACE_BEGIN

namespace dynamic_config {
namespace impl {

template <typename T>
using IsJson = std::enable_if_t<std::is_same_v<T, formats::json::Value>>;

}  // namespace impl

/// A type-erased config key-value pair
class KeyValue final {
 public:
  /// Uses the provided value directly. It can also be constructed in-place:
  /// @code
  /// {kMyConfig, {"foo", 42}}
  /// @endcode
  ///
  /// When passed a formats::json::Value, parses the value from it:
  /// @code
  /// {kMyConfig, formats::json::FromString(R"({"foo": "what", "bar": 42})")}
  /// @endcode
  template <typename VariableType, typename Value = VariableType>
  KeyValue(const Key<VariableType>& key, Value&& value)
      : id_(impl::ConfigIdGetter::Get(key)),
        value_(Convert<VariableType>(std::forward<Value>(value))) {}

  /// For internal use only
  impl::ConfigId GetId() const { return id_; }

  /// For internal use only
  std::any GetValue() const { return value_; }

 private:
  template <typename VariableType, typename Value>
  static VariableType Convert(Value&& value) {
    if constexpr (std::is_same_v<std::decay_t<Value>, formats::json::Value>) {
      return value.template As<VariableType>();
    } else {
      return static_cast<VariableType>(std::forward<Value>(value));
    }
  }

  impl::ConfigId id_;
  std::any value_;
};

/// @brief Backing storage for `dynamic_config::Source` in tests and benchmarks
/// @warning Make sure that `StorageMock` outlives all the acquired pointers!
/// @snippet core/src/dynamic_config/config_test.cpp  Sample StorageMock usage
/// @snippet core/src/dynamic_config/config_test.cpp  StorageMock from JSON
/// @see dynamic_config::GetDefaultSource
/// @see dynamic_config::MakeDefaultStorage
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
  /// @see dynamic_config::MakeDefaultStorage
  StorageMock(const DocsMap& defaults, const std::vector<KeyValue>& overrides);

  StorageMock(StorageMock&&) noexcept;
  StorageMock& operator=(StorageMock&&) noexcept;
  ~StorageMock();

  /// Update some config variables
  void Extend(const std::vector<KeyValue>& overrides);

  Source GetSource() const&;
  Snapshot GetSnapshot() const&;

  // Store the StorageMock in a variable before using
  Snapshot GetSource() && = delete;
  Snapshot GetSnapshot() && = delete;

 private:
  std::unique_ptr<impl::StorageData> storage_;
};

}  // namespace dynamic_config

USERVER_NAMESPACE_END
