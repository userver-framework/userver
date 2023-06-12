#pragma once

// Stub for logging support, required by TracefulException

USERVER_NAMESPACE_BEGIN

namespace logging::impl {

inline bool ShouldLogStacktrace() { return false; }

}  // namespace logging::impl

USERVER_NAMESPACE_END
