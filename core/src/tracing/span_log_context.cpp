#include <userver/tracing/span_log_context.hpp>

#include <ostream>

#include <userver/tracing/span.hpp>

USERVER_NAMESPACE_BEGIN

namespace tracing {

void SpanLogContext::SetFromSpan(const Span& span) {
    trace_id_.clear();
    span_id_.clear();
    link_.clear();
    trace_sampled_ = span.IsSampled();
    trace_id_ = span.GetTraceId();
    link_ = span.GetLink();
    if (const auto span_id = span.GetSpanIdForChildLogs()) {
        span_id_ = span_id.value();
    }
}

std::string_view SpanLogContext::GetTraceId() const noexcept { return trace_id_; }

std::string_view SpanLogContext::GetSpanId() const noexcept { return span_id_; }

std::string_view SpanLogContext::GetLink() const noexcept { return link_; }

bool SpanLogContext::IsEmpty() const noexcept { return !trace_sampled_.has_value(); }

bool SpanLogContext::IsSampled() const noexcept { return trace_sampled_.value_or(false); }

logging::LogExtra SpanLogContext::GetLogExtra() const {
    if (IsEmpty()) {
        return {};
    }
    logging::LogExtra context{{"trace_sampled", IsSampled()}};
    context.Extend("trace_id", std::string{GetTraceId()});
    if (!span_id_.empty()) {
        context.Extend("span_id", std::string{GetSpanId()});
    }
    context.Extend("link", std::string{GetLink()});
    return context;
}

void PrintTo(const SpanLogContext& context, std::ostream* os) {
    if (context.IsEmpty()) {
        *os << "SpanLogContext{}";
        return;
    }
    *os << "SpanLogContext{trace_id=" << context.GetTraceId() << ", span_id=" << context.GetSpanId()
        << ", link=" << context.GetLink() << ", trace_sampled=" << (context.IsSampled() ? "true" : "false") << '}';
}

}  // namespace tracing

USERVER_NAMESPACE_END
