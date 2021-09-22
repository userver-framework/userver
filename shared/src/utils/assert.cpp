#include <userver/utils/assert.hpp>

#include <iostream>

#include <fmt/format.h>
#include <fmt/ostream.h>
#include <boost/stacktrace.hpp>

#include <userver/logging/log.hpp>
#include <userver/utils/invariant_error.hpp>

namespace utils::impl {

void UASSERT_failed(std::string_view expr, const char* file, unsigned int line,
                    const char* function, std::string_view msg) noexcept {
  auto trace = boost::stacktrace::stacktrace();

  // Use fmt::format to output the message without interleaving with other logs.
  std::cerr << fmt::format(
      "ERROR at {}:{}:{}. Assertion '{}' failed{}{}. Stacktrace:\n{}\n", file,
      line, (function ? function : ""), expr,
      (msg.empty() ? std::string_view{} : std::string_view{": "}), msg, trace);

  logging::LogFlush();
  abort();
}

void LogAndThrowInvariantError(std::string_view condition,
                               std::string_view message) {
  const auto err_str =
      ::fmt::format("Invariant ({}) violation: {}", condition, message);

  LOG_ERROR() << err_str;
  throw InvariantError(err_str);
}

}  // namespace utils::impl

// Function definitions for defined BOOST_ENABLE_ASSERT_HANDLER
namespace boost {
void assertion_failed(char const* expr, char const* function, char const* file,
                      long line) {
  utils::impl::UASSERT_failed(expr, file, line, function, {});
}

void assertion_failed_msg(char const* expr, char const* msg,
                          char const* function, char const* file, long line) {
  utils::impl::UASSERT_failed(expr, file, line, function, msg);
}
}  // namespace boost
