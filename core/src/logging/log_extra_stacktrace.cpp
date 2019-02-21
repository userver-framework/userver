#include <logging/log_extra_stacktrace.hpp>

#include <string>

#include <logging/stacktrace_cache.hpp>

namespace logging {
namespace impl {
namespace {

const std::string kTraceKey = "stacktrace";

}  // namespace

LogExtra MakeLogExtraStacktrace(const boost::stacktrace::stacktrace& trace,
                                utils::Flags<LogExtraStacktraceFlags> flags) {
  LogExtra ret;
  ret.Extend(kTraceKey,
             (flags & LogExtraStacktraceFlags::kNoCache)
                 ? to_string(trace)
                 : stacktrace_cache::to_string(trace),
             (flags & LogExtraStacktraceFlags::kFrozen)
                 ? LogExtra::ExtendType::kFrozen
                 : LogExtra::ExtendType::kNormal);
  return ret;
}

}  // namespace impl
}  // namespace logging
