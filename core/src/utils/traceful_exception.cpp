#include <utils/traceful_exception.hpp>

#include <logging/stacktrace_cache.hpp>

namespace utils {

void TracefulExceptionBase::EnsureNullTerminated() {
  message_buffer_.reserve(message_buffer_.size() + 1);
  message_buffer_[message_buffer_.size()] = '\0';
}

const char* TracefulException::what() const noexcept {
#ifndef NDEBUG
  try {
    if (what_buffer_.empty()) {
      what_buffer_ = to_string(MessageBuffer()) + ", stacktrace:\n" +
                     logging::stacktrace_cache::to_string(Trace());
    }
    return what_buffer_.c_str();
  } catch (const std::exception&) {
    // fallback to safe output
  }
#endif  // NDEBUG
  return MessageBuffer().size() != 0 ? MessageBuffer().data()
                                     : std::exception::what();
}

}  // namespace utils
