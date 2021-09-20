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

/// @brief Extends the `log_extra` without checking if it should
void ExtendLogExtraWithStacktrace(
    LogExtra& log_extra, const boost::stacktrace::stacktrace&,
    utils::Flags<LogExtraStacktraceFlags> = {}) noexcept;

/// @brief Extends the `log_extra` without checking if it should
/// by current stack inside the function call
void ExtendLogExtraWithStacktrace(
    LogExtra& log_extra, utils::Flags<LogExtraStacktraceFlags> = {}) noexcept;

/// @brief Checks if Debug level logging is enabled since
/// logging stacktrace is slow
bool ShouldLogStacktrace() noexcept;

}  // namespace logging::impl
