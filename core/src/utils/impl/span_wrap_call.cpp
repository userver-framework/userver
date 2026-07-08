#include <userver/utils/async.hpp>

#include <tracing/span_impl.hpp>
#include <userver/tracing/in_place_span.hpp>
#include <userver/tracing/span.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::impl {

struct SpanWrapCall::Impl {
    explicit Impl(std::string&& name, const SourceLocation& location);

    tracing::InPlaceSpan span;
};

SpanWrapCall::Impl::Impl(std::string&& name, const SourceLocation& location)
    : span(std::move(name), tracing::InPlaceSpan::DetachedTag{}, location)
{}

SpanWrapCall::SpanWrapCall(std::string&& name, const SourceLocation& location, HideSpan hide_span)
    : pimpl_(std::move(name), location)
{
    if (hide_span == HideSpan::kYes) {
        pimpl_->span.Get().SetLogLevel(logging::Level::kNone);
    }
}

void SpanWrapCall::DoBeforeInvoke() { pimpl_->span.Get().AttachToCoroStack(); }

SpanWrapCall::~SpanWrapCall() = default;

}  // namespace utils::impl

USERVER_NAMESPACE_END
