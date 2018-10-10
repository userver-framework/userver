#include <components/tracer.hpp>
#include <tracing/jaeger.hpp>
#include <tracing/noop.hpp>
#include <tracing/tracer.hpp>
#include <tracing/tracing_variant.hpp>

namespace components {

Tracer::Tracer(const ComponentConfig& config, const ComponentContext&) {
  tracing::TracerPtr tracer;

  auto tracer_type = config.ParseString("tracer");
  if (tracer_type == "noop") {
    tracer = tracing::MakeNoopTracer();
  } else if (tracer_type == "jaeger") {
    tracer = tracing::MakeJaegerLogTracer();
  } else {
    throw std::runtime_error("Unknown tracer type: " + tracer_type);
  }

  tracing::Tracer::SetTracer(std::move(tracer));
}

}  // namespace components
