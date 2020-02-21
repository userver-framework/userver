#include <components/tracer.hpp>
#include <logging/component.hpp>
#include <tracing/noop.hpp>
#include <tracing/opentracing.hpp>
#include <tracing/tracer.hpp>

namespace components {

Tracer::Tracer(const ComponentConfig& config, const ComponentContext& context) {
  auto& logging_component = context.FindComponent<Logging>();
  try {
    auto opentracing_logger = logging_component.GetLogger("opentracing");
    tracing::SetOpentracingLogger(opentracing_logger);
    LOG_INFO() << "Opentracing enabled.";
  } catch (const std::exception& exception) {
    LOG_INFO() << "Opentracing logger not set: " << exception;
  }
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
