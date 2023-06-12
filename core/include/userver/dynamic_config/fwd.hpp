#pragma once

/// @file userver/dynamic_config/fwd.hpp
/// @brief Forward declarations of `dynamic_config` classes

USERVER_NAMESPACE_BEGIN

namespace components {

// NOLINTNEXTLINE(bugprone-forward-declaration-namespace)
class DynamicConfig;
class DynamicConfigUpdatesSinkBase;

}  // namespace components

namespace dynamic_config {

struct Diff;
class DocsMap;
class KeyValue;
class Snapshot;  // NOLINT(bugprone-forward-declaration-namespace)
class Source;

}  // namespace dynamic_config

USERVER_NAMESPACE_END
