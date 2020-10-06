#include <tracing/tracing.hpp>

#include <atomic>

#include <tracing/noop.hpp>
#include <utils/swappingsmart.hpp>
#include <utils/uuid4.hpp>

namespace tracing {

namespace {

auto& GlobalTracer() {
  static const std::string kEmptyServiceName;
  static utils::SwappingSmart<Tracer> tracer(
      tracing::MakeNoopTracer(kEmptyServiceName));
  return tracer;
}

}  // namespace

Tracer::~Tracer() = default;

void Tracer::SetTracer(std::shared_ptr<Tracer> tracer) {
  GlobalTracer().Set(tracer);
}

std::shared_ptr<Tracer> Tracer::GetTracer() { return GlobalTracer().Get(); }

const std::string& Tracer::GetServiceName() const { return service_name_; }

Span Tracer::CreateSpanWithoutParent(std::string name) {
  auto span =
      Span(shared_from_this(), std::move(name), nullptr, ReferenceType::kChild);

  span.SetLink(utils::generators::GenerateUuid());
  return span;
}

Span Tracer::CreateSpan(std::string name, const Span& parent,
                        ReferenceType reference_type) {
  return Span(shared_from_this(), std::move(name), &parent, reference_type);
}

}  // namespace tracing
