#include <userver/tracing/component.hpp>

#include <userver/components/component.hpp>
#include <userver/logging/component.hpp>
#include <userver/tracing/tracer.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

namespace {
constexpr std::string_view kNativeTrace = "native";
}

Tracer::Tracer(const ComponentConfig& config, const ComponentContext& context) {
  auto& logging_component = context.FindComponent<Logging>();
  auto opentracing_logger = logging_component.GetLoggerOptional("opentracing");
  auto service_name = config["service-name"].As<std::string>({});
  const auto tracer_type = config["tracer"].As<std::string>(kNativeTrace);
  if (tracer_type == kNativeTrace) {
    if (service_name.empty() && opentracing_logger) {
      throw std::runtime_error(
          "Opentracing logger was set, but the `service-name` is empty. "
          "Please provide a service name to use in OpenTracing");
    }

    if (opentracing_logger) {
      LOG_INFO() << "Opentracing enabled.";
    } else {
      LOG_INFO() << "Opentracing logger is not registered";
    }

    tracing::Tracer::SetTracer(tracing::MakeTracer(
        std::move(service_name), std::move(opentracing_logger), tracer_type));
  } else {
    throw std::runtime_error("Tracer type is not supported: " + tracer_type);
  }
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
        defaultDescription: ''
    tracer:
        type: string
        description: type of the tracer to trace, currently supported only 'native'
        defaultDescription: 'native'
)");
}

}  // namespace components

USERVER_NAMESPACE_END
