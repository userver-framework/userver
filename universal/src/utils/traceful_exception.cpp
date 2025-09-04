#include <userver/utils/traceful_exception.hpp>

#include <fmt/compile.h>
#include <fmt/format.h>
#include <boost/stacktrace/stacktrace.hpp>

#include <logging/log_extra_stacktrace.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

namespace {

boost::stacktrace::stacktrace CollectTrace(TracefulException::TraceMode trace_mode) {
    if (trace_mode == TracefulException::TraceMode::kIfLoggingIsEnabled && !logging::impl::ShouldLogStacktrace()) {
        return boost::stacktrace::stacktrace(0, 0);
    }
    return boost::stacktrace::stacktrace{};
}

}  // namespace

struct TracefulExceptionBase::Impl final {
    explicit Impl(TraceMode trace_mode) : stacktrace(CollectTrace(trace_mode)) {}

    MemoryBuffer message_buffer;
    boost::stacktrace::stacktrace stacktrace;
};

TracefulExceptionBase::TracefulExceptionBase(TracefulExceptionBase&& other) noexcept : impl_(std::move(other.impl_)) {
    EnsureNullTerminated();
}

TracefulExceptionBase::~TracefulExceptionBase() = default;

void TracefulExceptionBase::EnsureNullTerminated() {
    impl_->message_buffer.reserve(impl_->message_buffer.size() + 1);
    impl_->message_buffer[impl_->message_buffer.size()] = '\0';
}

const char* TracefulException::what() const noexcept {
    return MessageBuffer().size() != 0 ? MessageBuffer().data() : std::exception::what();
}

const TracefulExceptionBase::MemoryBuffer& TracefulExceptionBase::MessageBuffer() const noexcept {
    return impl_->message_buffer;
}

const boost::stacktrace::stacktrace& TracefulExceptionBase::Trace() const noexcept { return impl_->stacktrace; }

TracefulExceptionBase::TracefulExceptionBase() : TracefulExceptionBase(TraceMode::kAlways) {}

TracefulExceptionBase::TracefulExceptionBase(std::string_view what) : TracefulExceptionBase(TraceMode::kAlways) {
    fmt::format_to(std::back_inserter(impl_->message_buffer), FMT_COMPILE("{}"), what);
    EnsureNullTerminated();
}

TracefulExceptionBase::TracefulExceptionBase(TraceMode trace_mode) : impl_(trace_mode) {}

TracefulExceptionBase::MemoryBuffer& TracefulExceptionBase::GetMessageBuffer() { return impl_->message_buffer; }

}  // namespace utils

USERVER_NAMESPACE_END
