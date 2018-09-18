#include <components/tracer.hpp>
#include <tracing/tracer.hpp>

#include <opentracing/noop.h>

namespace components {

namespace {

tracing::TracerPtr MakeNoopTracer() {
  auto ot_tracer = opentracing::MakeNoopTracer();
  return std::make_shared<tracing::Tracer>(std::move(ot_tracer));
}

}  // namespace

Tracer::Tracer(const ComponentConfig& config, const ComponentContext&) {
  tracing::TracerPtr tracer;

  auto tracer_type = config.ParseString("tracer");
  if (tracer_type == "noop") {
    tracer = MakeNoopTracer();
  } else {
    throw std::runtime_error("Unknown tracer type: " + tracer_type);
  }

  tracing::Tracer::SetTracer(std::move(tracer));
}

}  // namespace components
