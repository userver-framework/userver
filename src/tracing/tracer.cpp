#include <tracing/tracing.hpp>

#include <atomic>

#include <opentracing/noop.h>

#include <tracing/noop.hpp>
#include <utils/uuid4.hpp>

namespace tracing {

namespace {

TracerPtr& GlobalTracer() {
  static TracerPtr tracer = tracing::MakeNoopTracer();
  return tracer;
}

}  // namespace

Tracer::Tracer(std::shared_ptr<opentracing::Tracer> tracer)
    : tracer_(std::move(tracer)) {}

Tracer::~Tracer() = default;

void Tracer::SetTracer(std::shared_ptr<Tracer> tracer) {
  std::atomic_store(&GlobalTracer(), std::move(tracer));
}

std::shared_ptr<Tracer> Tracer::GetTracer() {
  return std::atomic_load(&GlobalTracer());
}

Span Tracer::CreateSpanWithoutParent(const std::string& name) {
  auto ot_span = tracer_->StartSpan(name);
  auto span = Span(std::move(ot_span), shared_from_this(), name);

  span.SetLink(utils::generators::GenerateUuid());
  return span;
}

Span Tracer::CreateSpan(const std::string& name, const Span& parent) {
  auto ot_span = tracer_->StartSpan(
      name, {opentracing::ChildOf(&parent.GetOpentracingSpan().context())});
  return Span(std::move(ot_span), shared_from_this(), name);
}

}  // namespace tracing
