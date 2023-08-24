#include <userver/utils/assert.hpp>

#include <iostream>

#include <fmt/format.h>
#include <fmt/ostream.h>
#include <boost/stacktrace/stacktrace.hpp>

#include <userver/logging/log.hpp>
#include <userver/utils/invariant_error.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::impl {

void UASSERT_failed(std::string_view expr, const char* file, unsigned int line,
                    const char* function, std::string_view msg) noexcept {
  auto trace = dump_stacktrace_on_assert_failure
                   ? boost::stacktrace::stacktrace()
                   : boost::stacktrace::stacktrace(0, 0);

  // TODO pass file, line, function to LogHelper
  const auto message = fmt::format(
      "ERROR at {}:{}:{}. Assertion '{}' failed{}{}", file, line,
      (function ? function : ""), expr,
      (msg.empty() ? std::string_view{} : std::string_view{": "}), msg);
  LOG_CRITICAL() << message;
  logging::LogFlush();

  // Use fmt::format to output the message without interleaving with other logs.
  std::cerr << fmt::format("{}. Stacktrace:\n{}\n", message,
                           boost::stacktrace::to_string(trace))
            << std::flush;

  abort();
}

void LogAndThrowInvariantError(std::string_view condition,
                               std::string_view message) {
  const auto err_str =
      ::fmt::format("Invariant ({}) violation: {}", condition, message);

  LOG_ERROR() << err_str;
  throw InvariantError(err_str);
}

bool dump_stacktrace_on_assert_failure = true;

}  // namespace utils::impl

USERVER_NAMESPACE_END

// Function definitions for defined BOOST_ENABLE_ASSERT_HANDLER
namespace boost {
void assertion_failed(char const* expr, char const* function, char const* file,
                      long line) {
  USERVER_NAMESPACE::utils::impl::UASSERT_failed(expr, file, line, function,
                                                 {});
}

void assertion_failed_msg(char const* expr, char const* msg,
                          char const* function, char const* file, long line) {
  USERVER_NAMESPACE::utils::impl::UASSERT_failed(expr, file, line, function,
                                                 msg);
}
}  // namespace boost
