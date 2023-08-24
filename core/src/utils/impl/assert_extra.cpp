#include <utils/impl/assert_extra.hpp>

#include <iostream>

#include <fmt/format.h>
#include <boost/stacktrace.hpp>

#include <userver/logging/log.hpp>
#include <userver/logging/stacktrace_cache.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::impl {

[[noreturn]] void AbortWithStacktrace(std::string_view message) noexcept {
  UASSERT_MSG(false, message);
  LOG_CRITICAL() << message << logging::LogExtra::Stacktrace();
  logging::LogFlush();
  const auto trace = boost::stacktrace::stacktrace();
  std::cerr << fmt::format("{}. Stacktrace: {}", message,
                           logging::stacktrace_cache::to_string(trace));
  std::abort();
}

}  // namespace utils::impl

USERVER_NAMESPACE_END
