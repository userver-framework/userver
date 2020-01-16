#include <utils/assert.hpp>

#include <iostream>

#include <boost/stacktrace.hpp>

#include <logging/log.hpp>

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
