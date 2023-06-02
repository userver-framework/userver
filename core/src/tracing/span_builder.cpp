#include <userver/tracing/span_builder.hpp>

#include <tracing/span_impl.hpp>
#include <userver/tracing/tracer.hpp>
#include <userver/utils/impl/source_location.hpp>
#include <userver/utils/uuid4.hpp>

USERVER_NAMESPACE_BEGIN

namespace tracing {

SpanBuilder::SpanBuilder(std::string name,
                         const utils::impl::SourceLocation& location)
    : pimpl_(AllocateImpl(tracing::Tracer::GetTracer(), std::move(name),
                          GetParentSpanImpl(), ReferenceType::kChild,
                          logging::Level::kInfo, location),
             Span::OptionalDeleter{Span::OptionalDeleter::ShouldDelete()}) {
  pimpl_->AttachToCoroStack();
  if (pimpl_->GetParentId().empty()) {
    AddTagFrozen(kLinkTag, utils::generators::GenerateUuid());
  }
}

void SpanBuilder::SetSpanId(std::string parent_span_id) {
  pimpl_->SetSpanId(std::move(parent_span_id));
}

void SpanBuilder::SetParentSpanId(std::string parent_span_id) {
  pimpl_->SetParentId(std::move(parent_span_id));
}

void SpanBuilder::SetTraceId(std::string trace_id) {
  pimpl_->SetTraceId(std::move(trace_id));
}

const std::string& SpanBuilder::GetTraceId() const noexcept {
  return pimpl_->GetTraceId();
}

void SpanBuilder::AddTagFrozen(std::string key,
                               logging::LogExtra::Value value) {
  pimpl_->log_extra_inheritable_.Extend(std::move(key), std::move(value),
                                        logging::LogExtra::ExtendType::kFrozen);
}

void SpanBuilder::SetParentLink(std::string parent_link) {
  AddTagFrozen(kParentLinkTag, std::move(parent_link));
}

Span SpanBuilder::Build() && { return Span(std::move(pimpl_)); }

}  // namespace tracing

USERVER_NAMESPACE_END
