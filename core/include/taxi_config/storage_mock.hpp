#pragma once

#include <memory>

#include <taxi_config/config.hpp>
#include <taxi_config/config_ptr.hpp>
#include <taxi_config/value.hpp>

namespace taxi_config {

/// @brief Used to work with config smart pointers in tests and benchmarks
/// @warning Make sure that Storage outlives all the acquired pointers!
class StorageMock final {
 public:
  explicit StorageMock(const DocsMap& docs_map);

  StorageMock(StorageMock&&) noexcept = default;
  StorageMock& operator=(StorageMock&&) noexcept = default;

  template <typename T>
  SnapshotPtr<T> GetSnapshot() const {
    return GetSource<T>().Get();
  }

  template <typename T>
  Source<T> GetSource() const {
    return Source<T>{*storage_};
  }

  template <typename T>
  [[deprecated("Use GetSnapshot instead")]] SnapshotPtr<T> Get() const {
    return GetSnapshot<T>();
  }

  template <typename T>
  [[deprecated("Use GetSource instead")]] Source<T> GetVariable() const {
    return GetSource<T>();
  }

  std::shared_ptr<const Config> GetShared() const;

 private:
  std::unique_ptr<const impl::Storage> storage_;
};

}  // namespace taxi_config
