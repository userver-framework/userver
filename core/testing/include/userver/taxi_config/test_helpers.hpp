#pragma once

#include <string>
#include <vector>

#include <userver/taxi_config/source.hpp>
#include <userver/taxi_config/storage_mock.hpp>
#include <userver/taxi_config/test_helpers_impl.hpp>

USERVER_NAMESPACE_BEGIN

namespace taxi_config {

#if defined(DEFAULT_TAXI_CONFIG_FILENAME) || defined(DOXYGEN)
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

USERVER_NAMESPACE_END
