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

Tracer::Tracer(const ComponentConfig& config, const ComponentContext&) {
  auto service_name = config["service-name"].As<std::string>({});
  const auto tracer_type = config["tracer"].As<std::string>(kNativeTrace);
  if (tracer_type == kNativeTrace) {
    tracing::Tracer::SetTracer(tracing::MakeTracer(std::move(service_name)));
  } else {
    throw std::runtime_error("Tracer type is not supported: " + tracer_type);
  }
}

yaml_config::Schema Tracer::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<RawComponentBase>(R"(
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
