#include <userver/tracing/span_builder.hpp>

#include <tracing/span_impl.hpp>
#include <userver/tracing/tracer.hpp>
#include <userver/utils/impl/source_location.hpp>

USERVER_NAMESPACE_BEGIN

namespace tracing {

SpanBuilder::SpanBuilder(std::string name, const utils::impl::SourceLocation& location)
    : pimpl_(
          AllocateImpl(std::move(name), GetParentSpanImpl(), ReferenceType::kChild, location),
          Span::OptionalDeleter{Span::OptionalDeleter::ShouldDelete()}
      ) {
    // 1. If we AttachToCoroStack() in Build(), then any logs will not be attached to the trace for now.
    // 2. If we AttachToCoroStack() here, then logs may get attached to a "non-existent" span
    //    (logs will get `trace_id` that will change before Build()).
    // Let's do (1) for now.
}

void SpanBuilder::SetSpanId(std::string_view parent_span_id) { pimpl_->SetSpanId(parent_span_id); }

void SpanBuilder::SetLink(std::string_view link) { pimpl_->SetLink(link); }

void SpanBuilder::SetParentSpanId(std::string_view parent_span_id) { pimpl_->SetParentId(parent_span_id); }

void SpanBuilder::SetTraceId(std::string_view trace_id) { pimpl_->SetTraceId(trace_id); }

std::string_view SpanBuilder::GetTraceId() const noexcept { return pimpl_->GetTraceId(); }

void SpanBuilder::SetParentLink(std::string_view parent_link) { pimpl_->SetParentLink(parent_link); }

void SpanBuilder::AddTagFrozen(std::string key, logging::LogExtra::Value value) {
    pimpl_->log_extra_inheritable_.Extend(std::move(key), std::move(value), logging::LogExtra::ExtendType::kFrozen);
}

void SpanBuilder::AddNonInheritableTag(std::string key, logging::LogExtra::Value value) {
    if (!pimpl_->log_extra_local_) pimpl_->log_extra_local_.emplace();
    pimpl_->log_extra_local_->Extend(std::move(key), std::move(value));
}

Span SpanBuilder::Build() && {
    pimpl_->AttachToCoroStack();
    return Span(std::move(pimpl_));
}

Span SpanBuilder::BuildDetachedFromCoroStack() && { return Span(std::move(pimpl_)); }

}  // namespace tracing

USERVER_NAMESPACE_END
