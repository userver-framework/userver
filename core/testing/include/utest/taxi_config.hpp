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

// Internal function, use utest::MakeTaxiConfigPtr instead
utils::SharedReadablePtr<taxi_config::Config> MakeTaxiConfigPtr(
    const std::string& filename, const taxi_config::DocsMap& overrides);

}  // namespace impl

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
inline utils::SharedReadablePtr<taxi_config::Config> MakeTaxiConfigPtr(
    const taxi_config::DocsMap& overrides) {
  return impl::MakeTaxiConfigPtr(DEFAULT_TAXI_CONFIG_FILENAME, overrides);
}
#endif

}  // namespace utest
