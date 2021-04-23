#pragma once

#include <memory>
#include <utility>

#include <taxi_config/config.hpp>
#include <taxi_config/config_ptr.hpp>
#include <taxi_config/value.hpp>

namespace taxi_config {

/// @brief Used to work with config smart pointers in tests and benchmarks
/// @warning Make sure that `StorageMock` outlives all the acquired pointers!
class StorageMock final {
 public:
  /// Parse all config variables used in the service, as normal
  explicit StorageMock(const DocsMap& docs_map);

  /// Only store the given config variables in the `Config`
  /// Use as: `StorageMock{{kVariableKey1, value1}, {kVariableKey2, value2}}`
  StorageMock(std::initializer_list<Config::KeyValue> config_variables);

  StorageMock(StorageMock&&) noexcept = default;
  StorageMock& operator=(StorageMock&&) noexcept = default;

  const Source& operator*() const;
  const Source* operator->() const;

 private:
  std::unique_ptr<impl::Storage> storage_;
  Source source_;
};

}  // namespace taxi_config
