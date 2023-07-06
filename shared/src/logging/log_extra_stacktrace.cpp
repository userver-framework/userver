#include <logging/log_extra_stacktrace.hpp>

#include <string>

#include <boost/stacktrace.hpp>

#include <userver/logging/level.hpp>
#include <userver/logging/log.hpp>
#include <userver/logging/stacktrace_cache.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging::impl {
namespace {

const std::string kTraceKey = "stacktrace";

}  // namespace

void ExtendLogExtraWithStacktrace(
    LogExtra& log_extra, const boost::stacktrace::stacktrace& trace,
    utils::Flags<LogExtraStacktraceFlags> flags) noexcept {
  try {
    log_extra.Extend(kTraceKey,
                     (flags & LogExtraStacktraceFlags::kNoCache)
                         ? boost::stacktrace::to_string(trace)
                         : stacktrace_cache::to_string(trace),
                     (flags & LogExtraStacktraceFlags::kFrozen)
                         ? LogExtra::ExtendType::kFrozen
                         : LogExtra::ExtendType::kNormal);
  } catch (const std::exception& e) {
    UASSERT_MSG(false, e.what());
  }
}

void ExtendLogExtraWithStacktrace(
    LogExtra& log_extra, utils::Flags<LogExtraStacktraceFlags> flags) noexcept {
  ExtendLogExtraWithStacktrace(log_extra, boost::stacktrace::stacktrace{},
                               flags);
}

bool ShouldLogStacktrace() noexcept {
  return ShouldLog(logging::Level::kDebug);
}

bool LoggerShouldLogStacktrace(logging::LoggerCRef logger) noexcept {
  return LoggerShouldLog(logger, logging::Level::kDebug);
}

}  // namespace logging::impl

USERVER_NAMESPACE_END
