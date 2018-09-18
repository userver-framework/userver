#include <tracing/tracing.hpp>

#include <atomic>

#include <opentracing/noop.h>
#include <opentracing/tracer.h>

namespace tracing {

namespace {

TracerPtr& GlobalTracer() {
  static TracerPtr tracer =
      std::make_shared<Tracer>(opentracing::MakeNoopTracer());
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
  return Span(std::move(ot_span), shared_from_this());
}

Span Tracer::CreateSpan(const std::string& name, Span& parent) {
  auto ot_span = tracer_->StartSpan(
      name, {opentracing::ChildOf(&parent.GetOtSpan().context())});
  return Span(std::move(ot_span), shared_from_this());
}

}  // namespace tracing
