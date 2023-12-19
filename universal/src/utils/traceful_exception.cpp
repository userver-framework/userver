#include <userver/utils/traceful_exception.hpp>

#include <fmt/compile.h>
#include <fmt/format.h>
#include <boost/stacktrace/stacktrace.hpp>

#include <logging/log_extra_stacktrace.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

namespace {

boost::stacktrace::stacktrace CollectTrace(
    TracefulException::TraceMode trace_mode) {
  if (trace_mode == TracefulException::TraceMode::kIfLoggingIsEnabled &&
      !logging::impl::ShouldLogStacktrace()) {
    return boost::stacktrace::stacktrace(0, 0);
  }
  return boost::stacktrace::stacktrace{};
}

}  // namespace

struct TracefulExceptionBase::Impl final {
  explicit Impl(TraceMode trace_mode) : stacktrace_(CollectTrace(trace_mode)) {}

  MemoryBuffer message_buffer_;
  boost::stacktrace::stacktrace stacktrace_;
};

TracefulExceptionBase::TracefulExceptionBase(
    TracefulExceptionBase&& other) noexcept
    : impl_(std::move(other.impl_)) {
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

TracefulExceptionBase::TracefulExceptionBase()
    : TracefulExceptionBase(TraceMode::kAlways) {}

TracefulExceptionBase::TracefulExceptionBase(std::string_view what)
    : TracefulExceptionBase(TraceMode::kAlways) {
  fmt::format_to(std::back_inserter(impl_->message_buffer_), FMT_COMPILE("{}"),
                 what);
  EnsureNullTerminated();
}

TracefulExceptionBase::TracefulExceptionBase(TraceMode trace_mode)
    : impl_(trace_mode) {}

TracefulExceptionBase::MemoryBuffer& TracefulExceptionBase::GetMessageBuffer() {
  return impl_->message_buffer_;
}

}  // namespace utils

USERVER_NAMESPACE_END
