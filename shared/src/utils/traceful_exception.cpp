#include <userver/utils/traceful_exception.hpp>

#include <boost/stacktrace/stacktrace.hpp>

#include <fmt/compile.h>
#include <fmt/format.h>

USERVER_NAMESPACE_BEGIN

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

TracefulExceptionBase::TracefulExceptionBase() = default;

TracefulExceptionBase::TracefulExceptionBase(std::string_view what) {
  fmt::format_to(std::back_inserter(impl_->message_buffer_), FMT_COMPILE("{}"),
                 what);
  EnsureNullTerminated();
}

TracefulExceptionBase::MemoryBuffer& TracefulExceptionBase::GetMessageBuffer() {
  return impl_->message_buffer_;
}

}  // namespace utils

USERVER_NAMESPACE_END
