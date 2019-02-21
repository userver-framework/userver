#include <utils/traceful_exception.hpp>

#include <logging/stacktrace_cache.hpp>

namespace utils {

const char* TracefulException::what() const noexcept {
#ifndef NDEBUG
  try {
    if (what_buffer_.empty()) {
      what_buffer_ = Message() + ", stacktrace:\n" +
                     logging::stacktrace_cache::to_string(Trace());
    }
    return what_buffer_.c_str();
  } catch (const std::exception&) {
    // fallback to safe output
  }
#endif  // NDEBUG
  return Message().empty() ? std::exception::what() : Message().c_str();
}

}  // namespace utils
