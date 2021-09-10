#pragma once

#include <boost/stacktrace/stacktrace_fwd.hpp>

#include <userver/logging/log_extra.hpp>
#include <userver/utils/flags.hpp>

namespace logging::impl {

enum class LogExtraStacktraceFlags {
  kNone = 0,
  kNoCache = 1,
  kFrozen = (kNoCache << 1),
};

void ExtendLogExtraWithStacktrace(
    LogExtra& log_extra, const boost::stacktrace::stacktrace&,
    utils::Flags<LogExtraStacktraceFlags> = {}) noexcept;

bool ShouldLogStacktrace() noexcept;

}  // namespace logging::impl
