#include <components/tracer.hpp>
#include <tracing/noop.hpp>
#include <tracing/tracer.hpp>

namespace components {

Tracer::Tracer(const ComponentConfig& config, const ComponentContext&) {
  tracing::TracerPtr tracer;

  auto tracer_type = config.ParseString("tracer");
  if (tracer_type == "native") {
    tracer = tracing::MakeNoopTracer();
  } else {
    throw std::runtime_error("Tracer type is not supported: " + tracer_type);
  }

  // All other tracers were removed in this PR:
  // https://github.yandex-team.ru/taxi/userver/pull/206

  tracing::Tracer::SetTracer(std::move(tracer));
}

}  // namespace components
