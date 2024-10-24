#include <userver/ugrpc/server/middlewares/field_mask_bin/component.hpp>

#include <ugrpc/server/middlewares/field_mask_bin/middleware.hpp>
#include <userver/components/component_config.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::middlewares::field_mask_bin {

Component::Component(const components::ComponentConfig& config, const components::ComponentContext& context)
    : MiddlewareComponentBase(config, context),
      metadata_field_name_(config["metadata-field-name"].As<std::string>(kDefaultMetadataFieldName)) {}

std::shared_ptr<MiddlewareBase> Component::GetMiddleware() {
    return std::make_shared<Middleware>(metadata_field_name_);
}

yaml_config::Schema Component::GetStaticConfigSchema() {
    return yaml_config::MergeSchemas<MiddlewareComponentBase>(R"(
type: object
description: gRPC service field-mask-bin middleware component
additionalProperties: false
properties:
    metadata-field-name:
        type: string
        description: name of the metadata field to get the field mask from
)");
}

}  // namespace ugrpc::server::middlewares::field_mask_bin

USERVER_NAMESPACE_END
