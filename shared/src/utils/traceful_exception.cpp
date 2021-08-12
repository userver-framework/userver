#include <userver/utils/traceful_exception.hpp>

#include <boost/stacktrace/stacktrace.hpp>

namespace utils {

struct TracefulExceptionBase::Impl {
  MemoryBuffer message_buffer_;
  boost::stacktrace::stacktrace stacktrace_;

  Impl(MemoryBuffer message_buffer, boost::stacktrace::stacktrace stacktrace)
      : message_buffer_(std::move(message_buffer)),
        stacktrace_(std::move(stacktrace)) {}

  Impl() = default;
};

TracefulExceptionBase::TracefulExceptionBase(
    TracefulExceptionBase&& other) noexcept
    : impl_(std::move(other.impl_->message_buffer_),
            std::move(other.impl_->stacktrace_)) {
  EnsureNullTerminated();
}

TracefulExceptionBase::~TracefulExceptionBase() = default;

void TracefulExceptionBase::EnsureNullTerminated() {
  impl_->message_buffer_.reserve(impl_->message_buffer_.size() + 1);
  impl_->message_buffer_[impl_->message_buffer_.size()] = '\0';
}

const char* TracefulException::what() const noexcept {
  return MessageBuffer().size() != 0 ? MessageBuffer().data()
                                     : std::exception::what();
}

const TracefulExceptionBase::MemoryBuffer&
TracefulExceptionBase::MessageBuffer() const noexcept {
  return impl_->message_buffer_;
}

const boost::stacktrace::stacktrace& TracefulExceptionBase::Trace() const
    noexcept {
  return impl_->stacktrace_;
}

TracefulExceptionBase::TracefulExceptionBase() : impl_() {}

TracefulExceptionBase::TracefulExceptionBase(std::string_view what) : impl_() {
  fmt::format_to(impl_->message_buffer_, "{}", what);
  EnsureNullTerminated();
}

TracefulExceptionBase::MemoryBuffer& TracefulExceptionBase::GetMessageBuffer() {
  return impl_->message_buffer_;
}

}  // namespace utils
