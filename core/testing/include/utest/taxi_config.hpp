#pragma once

#include <taxi_config/snapshot.hpp>
#include <taxi_config/source.hpp>
#include <taxi_config/test_helpers.hpp>

namespace utest {

#ifdef DEFAULT_TAXI_CONFIG_FILENAME
[[deprecated("Use taxi_config::GetDefaultSnapshot() instead")]] inline std::
    shared_ptr<const taxi_config::Snapshot>
    GetDefaultTaxiConfigPtr() {
  return std::make_shared<const taxi_config::Snapshot>(
      taxi_config::GetDefaultSnapshot());
}
[
    [deprecated("Use taxi_config::GetDefaultSnapshot() "
                "instead")]] inline const taxi_config::Snapshot&
GetDefaultTaxiConfig() {
  return taxi_config::GetDefaultSnapshot();
}
#endif

}  // namespace utest
