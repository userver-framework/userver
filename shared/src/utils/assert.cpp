#include <userver/utils/assert.hpp>

#include <iostream>

#include <boost/stacktrace.hpp>

#include <userver/logging/log.hpp>

namespace utils {

void UASSERT_failed(const std::string& expr, const char* file,
                    unsigned int line, const char* function,
                    const std::string& msg) noexcept {
  auto trace = boost::stacktrace::stacktrace();

  std::cerr << file << ":" << line << ":" << (function ? function : "")
            << ": Assertion '" << expr << "' failed"
            << (msg.empty() ? std::string{} : (": " + msg))
            << ".  Stacktrace:\n"
            << trace << "\n";

  logging::LogFlush();
  abort();
}

void LogAndThrowInvariantError(const std::string& error) {
  LOG_ERROR() << error;
  throw InvariantError(error);
}

}  // namespace utils

// Function definitions for defined BOOST_ENABLE_ASSERT_HANDLER
namespace boost {
void assertion_failed(char const* expr, char const* function, char const* file,
                      long line) {
  utils::UASSERT_failed(expr, file, line, function, {});
}

void assertion_failed_msg(char const* expr, char const* msg,
                          char const* function, char const* file, long line) {
  utils::UASSERT_failed(expr, file, line, function, msg);
}
}  // namespace boost
