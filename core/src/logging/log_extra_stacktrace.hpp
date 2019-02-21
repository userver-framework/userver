#pragma once

#include <boost/stacktrace.hpp>

#include <logging/log_extra.hpp>
#include <utils/flags.hpp>

namespace logging {
namespace impl {

enum class LogExtraStacktraceFlags {
  kNone = 0,
  kNoCache = 1,
  kFrozen = (kNoCache << 1),
};

LogExtra MakeLogExtraStacktrace(const boost::stacktrace::stacktrace&,
                                utils::Flags<LogExtraStacktraceFlags> = {});

}  // namespace impl
}  // namespace logging
