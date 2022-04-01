#include <userver/components/impl/component_base.hpp>

#include <userver/yaml_config/merge_schemas.hpp>
#include <userver/yaml_config/schema.hpp>

USERVER_NAMESPACE_BEGIN

namespace components::impl {

// Putting detructor into a cpp file to force vtable instantiation in only 1
// translation unit
ComponentBase::~ComponentBase() = default;

yaml_config::Schema ComponentBase::GetStaticConfigSchema() {
  return yaml_config::impl::SchemaFromString(R"(
type: object
description: base component. Don't use it for application components, use LoggableComponentBase instead
additionalProperties: false
properties:
    load-enabled:
        type: boolean
        description: set to `false` to disable loading of the component
        defaultDescription: true
)");
}

}  // namespace components::impl

USERVER_NAMESPACE_END
