#pragma once

#include <string>
#include <vector>

#include <benchmark/benchmark.h>

#include <taxi_config/source.hpp>
#include <taxi_config/storage_mock.hpp>
#include <taxi_config/test_helpers_impl.hpp>

namespace taxi_config {

#ifdef DEFAULT_TAXI_CONFIG_FILENAME
/// Get `taxi_config::Source` with built-in defaults for all configs
inline taxi_config::Source GetDefaultSource() {
  return impl::GetDefaultSource(DEFAULT_TAXI_CONFIG_FILENAME);
}

/// Get `taxi_config::Snapshot` with built-in defaults for all configs
inline const taxi_config::Snapshot& GetDefaultSnapshot() {
  return impl::GetDefaultSnapshot(DEFAULT_TAXI_CONFIG_FILENAME);
}

/// Make `taxi_config::StorageMock` with built-in defaults for all configs
inline taxi_config::StorageMock MakeDefaultStorage(
    const std::vector<taxi_config::KeyValue>& overrides) {
  return impl::MakeDefaultStorage(DEFAULT_TAXI_CONFIG_FILENAME, overrides);
}
#endif

}  // namespace taxi_config
