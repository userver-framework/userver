#include <logging/log_extra_stacktrace.hpp>

#include <string>

#include <fmt/format.h>
#include <fmt/ostream.h>
#include <boost/stacktrace.hpp>

#include <userver/logging/level.hpp>
#include <userver/logging/stacktrace_cache.hpp>

namespace logging::impl {
namespace {

const std::string kTraceKey = "stacktrace";

}  // namespace

void ExtendLogExtraWithStacktrace(LogExtra& log_extra,
                                  const boost::stacktrace::stacktrace& trace,
                                  utils::Flags<LogExtraStacktraceFlags> flags) {
  if (!ShouldLogStacktrace()) {
    return;
  }

  log_extra.Extend(kTraceKey,
                   (flags & LogExtraStacktraceFlags::kNoCache)
                       // YA_COMPAT: old boost didn't have to_string for trace
                       ? fmt::to_string(trace)
                       : stacktrace_cache::to_string(trace),
                   (flags & LogExtraStacktraceFlags::kFrozen)
                       ? LogExtra::ExtendType::kFrozen
                       : LogExtra::ExtendType::kNormal);
}

bool ShouldLogStacktrace() noexcept {
  return ShouldLog(logging::Level::kDebug);
}

}  // namespace logging::impl
