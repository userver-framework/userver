#pragma once

/// @file userver/dynamic_config/test_helpers.hpp
/// @brief Accessors for dynamic config defaults for tests and benchmarks

#include <vector>

#include <userver/dynamic_config/snapshot.hpp>
#include <userver/dynamic_config/source.hpp>
#include <userver/dynamic_config/storage_mock.hpp>

USERVER_NAMESPACE_BEGIN

namespace dynamic_config {

/// Get `dynamic_config::Source` with built-in defaults for all configs
dynamic_config::Source GetDefaultSource();

/// Get `dynamic_config::Snapshot` with built-in defaults for all configs
const dynamic_config::Snapshot& GetDefaultSnapshot();

/// Make `dynamic_config::StorageMock` with built-in defaults for all configs
dynamic_config::StorageMock MakeDefaultStorage(
    const std::vector<dynamic_config::KeyValue>& overrides);

namespace impl {

// Internal API, use functions above instead!
const dynamic_config::DocsMap& GetDefaultDocsMap();

}  // namespace impl

}  // namespace dynamic_config

USERVER_NAMESPACE_END
