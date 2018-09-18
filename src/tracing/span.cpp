#include <tracing/span.hpp>

#include <opentracing/tracer.h>

#include <tracing/tracer.hpp>

namespace tracing {

Span::Span(std::unique_ptr<opentracing::Span> span, TracerPtr tracer)
    : span_(std::move(span)), tracer_(tracer) {}

Span::Span(Span&& other) noexcept
    : span_(std::move(other.span_)), tracer_(std::move(other.tracer_)) {}

Span::~Span() = default;

Span Span::CreateChild(const std::string& name) {
  return tracer_->CreateSpan(name, *this);
}

opentracing::Span& Span::GetOtSpan() { return *span_; }

}  // namespace tracing
