#include <userver/utils/assert.hpp>

#include <iostream>

#include <fmt/format.h>
#include <boost/stacktrace.hpp>

#include <userver/logging/log.hpp>
#include <userver/utils/invariant_error.hpp>

namespace utils::impl {

void UASSERT_failed(std::string_view expr, const char* file, unsigned int line,
                    const char* function, std::string_view msg) noexcept {
  auto trace = boost::stacktrace::stacktrace();

  std::cerr << file << ":" << line << ":" << (function ? function : "")
            << ": Assertion '" << expr << "' failed"
            << (msg.empty() ? std::string_view{} : std::string_view{": "})
            << msg << ".  Stacktrace:\n"
            << trace << "\n";

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
