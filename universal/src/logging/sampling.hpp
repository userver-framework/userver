#pragma once

#include <userver/logging/level.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging::impl {

// Returns true if the log record should be dropped due to log sampling.
// Never drops records of Level::kWarning and above.
bool ShouldDropBySampling(Level level) noexcept;

}  // namespace logging::impl

USERVER_NAMESPACE_END
