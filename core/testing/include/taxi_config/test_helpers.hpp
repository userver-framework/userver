#pragma once

#include <rcu/rcu.hpp>
#include <taxi_config/config.hpp>
#include <taxi_config/config_ptr.hpp>
#include <taxi_config/value.hpp>

namespace taxi_config {

/// @brief Used to work with config smart pointers in tests
/// @warning Make sure that Storage outlives all the acquired pointers!
class Storage {
 public:
  explicit Storage(const DocsMap& docs_map);

  template <typename T>
  ReadablePtr<T> Get() const {
    return GetVariable<T>().Get();
  }

  template <typename T>
  Variable<T> GetVariable() const {
    return Variable<T>{storage_};
  }

  std::shared_ptr<const Config> GetShared() const;

 private:
  impl::Storage storage_;
};

}  // namespace taxi_config
