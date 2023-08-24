#include <userver/components/tracer.hpp>

#include <userver/components/component.hpp>
#include <userver/logging/component.hpp>
#include <userver/tracing/noop.hpp>
#include <userver/tracing/opentracing.hpp>
#include <userver/tracing/tracer.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

namespace {
constexpr std::string_view kNativeTrace = "native";
}

Tracer::Tracer(const ComponentConfig& config, const ComponentContext& context) {
  auto& logging_component = context.FindComponent<Logging>();
  auto service_name = config["service-name"].As<std::string>();

  auto opentracing_logger = logging_component.GetLoggerOptional("opentracing");
  if (opentracing_logger) {
    tracing::SetOpentracingLogger(std::move(opentracing_logger));
    LOG_INFO() << "Opentracing enabled.";
  } else {
    LOG_INFO() << "Opentracing logger is not registered";
  }

  tracing::TracerPtr tracer;

  const auto tracer_type = config["tracer"].As<std::string>(kNativeTrace);
  if (tracer_type == kNativeTrace) {
    tracer = tracing::MakeNoopTracer(service_name);
  } else {
    throw std::runtime_error("Tracer type is not supported: " + tracer_type);
  }

  tracing::Tracer::SetTracer(std::move(tracer));
}

yaml_config::Schema Tracer::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<impl::ComponentBase>(R"(
type: object
description: Component that initializes the request tracing facilities.
additionalProperties: false
properties:
    service-name:
        type: string
        description: name of the service to write in traces
    tracer:
        type: string
        description: type of the tracer to trace, currently supported only 'native'
        defaultDescription: 'native'
)");
}

}  // namespace components

USERVER_NAMESPACE_END
