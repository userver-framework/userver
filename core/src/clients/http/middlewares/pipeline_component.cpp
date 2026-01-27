#include <userver/clients/http/middlewares/pipeline_component.hpp>

#include <userver/components/component_config.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#ifndef ARCADIA_ROOT
#include "generated/src/clients/http/middlewares/pipeline_component.yaml.hpp"  // Y_IGNORE
#endif

USERVER_NAMESPACE_BEGIN

namespace clients::http {

MiddlewarePipelineComponent::MiddlewarePipelineComponent(
    const components::ComponentConfig& config,
    const components::ComponentContext& context
)
    : components::ComponentBase(config, context),
      config_(config.As<USERVER_NAMESPACE::middlewares::impl::MiddlewaresConfig>())
{}

const USERVER_NAMESPACE::middlewares::impl::MiddlewaresConfig& MiddlewarePipelineComponent::GetMiddlewaresConfig()
    const {
    return config_;
}

yaml_config::Schema MiddlewarePipelineComponent::GetStaticConfigSchema() {
    return yaml_config::MergeSchemasFromResource<ComponentBase>("src/clients/http/middlewares/pipeline_component.yaml");
}

}  // namespace clients::http

USERVER_NAMESPACE_END
