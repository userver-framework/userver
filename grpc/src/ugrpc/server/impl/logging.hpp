#pragma once

#include <optional>

#include <userver/logging/level.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::impl {

// SetupLogging is thread-safe. It can be called multiple times if needed. Only
// kDebug, kInfo, kError levels are allowed.
void SetupLogging(logging::Level min_log_level_override);

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
