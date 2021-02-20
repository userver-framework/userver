#include <components/tracer.hpp>
#include <logging/component.hpp>
#include <tracing/noop.hpp>
#include <tracing/opentracing.hpp>
#include <tracing/tracer.hpp>

namespace components {

namespace {
const std::string kNativeTrace = "native";
}

Tracer::Tracer(const ComponentConfig& config, const ComponentContext& context) {
  auto& logging_component = context.FindComponent<Logging>();
  auto service_name = config["service-name"].As<std::string>();
  try {
    auto opentracing_logger = logging_component.GetLogger("opentracing");
    tracing::SetOpentracingLogger(opentracing_logger);
    LOG_INFO() << "Opentracing enabled.";
  } catch (const std::exception& exception) {
    LOG_INFO() << "Opentracing logger not set: " << exception;
  }
  tracing::TracerPtr tracer;

  const auto tracer_type = config["tracer"].As<std::string>(kNativeTrace);
  if (tracer_type == kNativeTrace) {
    tracer = tracing::MakeNoopTracer(service_name);
  } else {
    throw std::runtime_error("Tracer type is not supported: " + tracer_type);
  }

  // All other tracers were removed in this PR:
  // https://github.yandex-team.ru/taxi/userver/pull/206

  tracing::Tracer::SetTracer(std::move(tracer));
}

}  // namespace components
