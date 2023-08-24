#pragma once

#include <optional>

#include <userver/logging/level.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

// Thread-safe. Can be called multiple times if needed.
void SetupNativeLogging();

// Thread-safe. Can be called multiple times if needed, the most verbose log
// level is chosen. Only kDebug, kInfo, kError levels are allowed.
void UpdateNativeLogLevel(logging::Level min_log_level);

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
