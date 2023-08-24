#pragma once

/// @file userver/dynamic_config/test_helpers.hpp
/// @brief Contains getters for dynamic config defaults for tests.

#include <userver/dynamic_config/impl/test_helpers.hpp>

#ifdef ARCADIA_ROOT
#include "default_config_path.hpp"
#endif

USERVER_NAMESPACE_BEGIN

namespace dynamic_config {

#if defined(DEFAULT_DYNAMIC_CONFIG_FILENAME) || defined(DOXYGEN)
/// Get `dynamic_config::Source` with built-in defaults for all configs
inline dynamic_config::Source GetDefaultSource() {
  return dynamic_config::impl::GetDefaultSource(
      DEFAULT_DYNAMIC_CONFIG_FILENAME);
}

/// Get `dynamic_config::Snapshot` with built-in defaults for all configs
inline const dynamic_config::Snapshot& GetDefaultSnapshot() {
  return dynamic_config::impl::GetDefaultSnapshot(
      DEFAULT_DYNAMIC_CONFIG_FILENAME);
}

/// Make `dynamic_config::StorageMock` with built-in defaults for all configs
inline dynamic_config::StorageMock MakeDefaultStorage(
    const std::vector<dynamic_config::KeyValue>& overrides) {
  return dynamic_config::impl::MakeDefaultStorage(
      DEFAULT_DYNAMIC_CONFIG_FILENAME, overrides);
}

namespace impl {

// Internal API, use functions above instead!
inline const dynamic_config::DocsMap& GetDefaultDocsMap() {
  return dynamic_config::impl::GetDefaultDocsMap(
      DEFAULT_DYNAMIC_CONFIG_FILENAME);
}

}  // namespace impl
#endif

}  // namespace dynamic_config

USERVER_NAMESPACE_END
