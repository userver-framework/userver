#pragma once

#include <taxi_config/config.hpp>
#include <taxi_config/config_ptr.hpp>
#include <taxi_config/test_helpers.hpp>
#include <utils/shared_readable_ptr.hpp>

namespace utest {
namespace impl {

// Internal function, don't use it directly, use DefaultTaxiConfig() instead
std::shared_ptr<const taxi_config::Config> ReadDefaultTaxiConfigPtr(
    const std::string& filename);

// Internal function, use utest::MakeTaxiConfigPtr instead
utils::SharedReadablePtr<taxi_config::Config> MakeTaxiConfigPtr(
    const std::string& filename, const taxi_config::DocsMap& overrides);

}  // namespace impl

#ifdef DEFAULT_TAXI_CONFIG_FILENAME
inline std::shared_ptr<const taxi_config::Config> GetDefaultTaxiConfigPtr() {
  return impl::ReadDefaultTaxiConfigPtr(DEFAULT_TAXI_CONFIG_FILENAME);
}
inline const taxi_config::Config& GetDefaultTaxiConfig() {
  return *GetDefaultTaxiConfigPtr();
}
[[deprecated(
    "Use taxi_config::GetDefaultSource instead")]] inline taxi_config::Source
GetDefaultTaxiConfigSource() {
  return taxi_config::GetDefaultSource();
}
[[deprecated("Use taxi_config::MakeDefaultStorage instead")]] inline utils::
    SharedReadablePtr<taxi_config::Config>
    MakeTaxiConfigPtr(const taxi_config::DocsMap& overrides) {
  return impl::MakeTaxiConfigPtr(DEFAULT_TAXI_CONFIG_FILENAME, overrides);
}
#endif

}  // namespace utest
