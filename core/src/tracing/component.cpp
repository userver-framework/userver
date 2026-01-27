#include <userver/tracing/component.hpp>

#include <userver/components/component.hpp>
#include <userver/logging/component.hpp>
#include <userver/tracing/tracer.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#ifndef ARCADIA_ROOT
#include "generated/src/tracing/component.yaml.hpp"  // Y_IGNORE
#endif

USERVER_NAMESPACE_BEGIN

namespace components {

namespace {
constexpr std::string_view kNativeTrace = "native";
}

Tracer::Tracer(const ComponentConfig& config, const ComponentContext&) {
    auto service_name = config["service-name"].As<std::string>({});
    const auto tracer_type = config["tracer"].As<std::string>(kNativeTrace);
    if (tracer_type == kNativeTrace) {
        tracing::Tracer::SetTracer(tracing::Tracer(std::move(service_name)));
    } else {
        throw std::runtime_error("Tracer type is not supported: " + tracer_type);
    }
}

yaml_config::Schema Tracer::GetStaticConfigSchema() {
    return yaml_config::MergeSchemasFromResource<RawComponentBase>("src/tracing/component.yaml");
}

}  // namespace components

USERVER_NAMESPACE_END
