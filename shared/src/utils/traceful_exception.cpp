#include <utils/traceful_exception.hpp>

namespace utils {

TracefulExceptionBase::TracefulExceptionBase(
    TracefulExceptionBase&& other) noexcept
    : message_buffer_(std::move(other.message_buffer_)),
      stacktrace_(std::move(other.stacktrace_)) {
  EnsureNullTerminated();
}

void TracefulExceptionBase::EnsureNullTerminated() {
  message_buffer_.reserve(message_buffer_.size() + 1);
  message_buffer_[message_buffer_.size()] = '\0';
}

const char* TracefulException::what() const noexcept {
  return MessageBuffer().size() != 0 ? MessageBuffer().data()
                                     : std::exception::what();
}

}  // namespace utils
