#pragma once

#include <taxi_config/snapshot.hpp>
#include <taxi_config/source.hpp>
#include <taxi_config/test_helpers.hpp>
#include <utils/shared_readable_ptr.hpp>

namespace utest {
namespace impl {

// Internal function, don't use it directly, use DefaultTaxiConfig() instead
std::shared_ptr<const taxi_config::Snapshot> ReadDefaultTaxiConfigPtr(
    const std::string& filename);

// Internal function, use utest::MakeTaxiConfigPtr instead
utils::SharedReadablePtr<taxi_config::Snapshot> MakeTaxiConfigPtr(
    const std::string& filename, const taxi_config::DocsMap& overrides);

}  // namespace impl

#ifdef DEFAULT_TAXI_CONFIG_FILENAME
inline std::shared_ptr<const taxi_config::Snapshot> GetDefaultTaxiConfigPtr() {
  return impl::ReadDefaultTaxiConfigPtr(DEFAULT_TAXI_CONFIG_FILENAME);
}
inline const taxi_config::Snapshot& GetDefaultTaxiConfig() {
  return *GetDefaultTaxiConfigPtr();
}
[[deprecated(
    "Use taxi_config::GetDefaultSource instead")]] inline taxi_config::Source
GetDefaultTaxiConfigSource() {
  return taxi_config::GetDefaultSource();
}
[[deprecated("Use taxi_config::MakeDefaultStorage instead")]] inline utils::
    SharedReadablePtr<taxi_config::Snapshot>
    MakeTaxiConfigPtr(const taxi_config::DocsMap& overrides) {
  return impl::MakeTaxiConfigPtr(DEFAULT_TAXI_CONFIG_FILENAME, overrides);
}
#endif

}  // namespace utest
