#include <userver/components/raw_component_base.hpp>

#include <userver/yaml_config/merge_schemas.hpp>
#include <userver/yaml_config/schema.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

// Putting detructor into a cpp file to force vtable instantiation in only 1
// translation unit
RawComponentBase::~RawComponentBase() = default;

yaml_config::Schema RawComponentBase::GetStaticConfigSchema() {
  return yaml_config::impl::SchemaFromString(R"(
type: object
description: base component. Don't use it for application components, use ComponentBase instead
additionalProperties: false
properties:
    load-enabled:
        type: boolean
        description: set to `false` to disable loading of the component
        defaultDescription: true
)");
}

}  // namespace components

USERVER_NAMESPACE_END
