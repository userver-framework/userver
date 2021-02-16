#pragma once

#include <memory>

#include <taxi_config/config.hpp>
#include <taxi_config/config_ptr.hpp>
#include <taxi_config/value.hpp>

namespace taxi_config {

/// @brief Used to work with config smart pointers in tests and benchmarks
/// @warning Make sure that Storage outlives all the acquired pointers!
class StorageMock {
 public:
  explicit StorageMock(const DocsMap& docs_map);

  StorageMock(StorageMock&&) noexcept = default;
  StorageMock& operator=(StorageMock&&) noexcept = default;

  template <typename T>
  ReadablePtr<T> Get() const {
    return GetVariable<T>().Get();
  }

  template <typename T>
  Variable<T> GetVariable() const {
    return Variable<T>{*storage_};
  }

  std::shared_ptr<const Config> GetShared() const;

 private:
  std::unique_ptr<impl::Storage> storage_;
};

}  // namespace taxi_config
