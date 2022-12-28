#pragma once

#include <string>
#include <vector>

#include <benchmark/benchmark.h>

#include <userver/dynamic_config/impl/test_helpers.hpp>
#include <userver/dynamic_config/source.hpp>
#include <userver/dynamic_config/storage_mock.hpp>

USERVER_NAMESPACE_BEGIN

namespace dynamic_config {

#if defined(DEFAULT_DYNAMIC_CONFIG_FILENAME) || defined(DOXYGEN)
/// Get `dynamic_config::Source` with built-in defaults for all configs
inline dynamic_config::Source GetDefaultSource() {
  return impl::GetDefaultSource(DEFAULT_DYNAMIC_CONFIG_FILENAME);
}

/// Get `dynamic_config::Snapshot` with built-in defaults for all configs
inline const dynamic_config::Snapshot& GetDefaultSnapshot() {
  return impl::GetDefaultSnapshot(DEFAULT_DYNAMIC_CONFIG_FILENAME);
}

/// Make `dynamic_config::StorageMock` with built-in defaults for all configs
inline dynamic_config::StorageMock MakeDefaultStorage(
    const std::vector<dynamic_config::KeyValue>& overrides) {
  return impl::MakeDefaultStorage(DEFAULT_DYNAMIC_CONFIG_FILENAME, overrides);
}
#endif

}  // namespace dynamic_config

USERVER_NAMESPACE_END
