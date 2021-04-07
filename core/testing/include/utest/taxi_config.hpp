#pragma once

#include <taxi_config/config.hpp>
#include <taxi_config/config_ptr.hpp>
#include <utils/shared_readable_ptr.hpp>

namespace utest {
namespace impl {

// Internal function, don't use it directly, use DefaultTaxiConfig() instead
std::shared_ptr<const taxi_config::Config> ReadDefaultTaxiConfigPtr(
    const std::string& filename);

// Internal function, use GetDefaultTaxiConfigVariable() instead
taxi_config::Source ReadDefaultTaxiConfigSource(const std::string& filename);

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
inline taxi_config::Source GetDefaultTaxiConfigSource() {
  return impl::ReadDefaultTaxiConfigSource(DEFAULT_TAXI_CONFIG_FILENAME);
}
#endif

}  // namespace utest
