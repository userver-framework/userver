#pragma once

#include <taxi_config/config.hpp>
#include <taxi_config/config_ptr.hpp>
#include <taxi_config/storage_mock.hpp>
#include <utils/shared_readable_ptr.hpp>

namespace utest {
namespace impl {

// Internal function, don't use it directly, use DefaultTaxiConfig() instead
std::shared_ptr<const taxi_config::Config> ReadDefaultTaxiConfigPtr(
    const std::string& filename);

// Internal function, use GetDefaultTaxiConfigVariable() instead
const taxi_config::StorageMock& ReadDefaultTaxiConfigStorage(
    const std::string& filename);

}  // namespace impl

utils::SharedReadablePtr<taxi_config::Config> MakeTaxiConfigPtr(
    const taxi_config::DocsMap& docs_map);

/// Get taxi_config::Config with default values
#ifdef DEFAULT_TAXI_CONFIG_FILENAME
inline std::shared_ptr<const taxi_config::Config> GetDefaultTaxiConfigPtr() {
  return impl::ReadDefaultTaxiConfigPtr(DEFAULT_TAXI_CONFIG_FILENAME);
}
inline const taxi_config::Config& GetDefaultTaxiConfig() {
  return *GetDefaultTaxiConfigPtr();
}

template <typename T>
taxi_config::Variable<T> GetDefaultTaxiConfigVariable() {
  return impl::ReadDefaultTaxiConfigStorage(DEFAULT_TAXI_CONFIG_FILENAME)
      .GetVariable<T>();
}
#endif

}  // namespace utest
