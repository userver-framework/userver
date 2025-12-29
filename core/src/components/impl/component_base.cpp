#include <userver/components/raw_component_base.hpp>

#include <userver/utils/resources.hpp>
#include <userver/yaml_config/merge_schemas.hpp>
#include <userver/yaml_config/schema.hpp>

#ifndef ARCADIA_ROOT
#include "generated/src/components/impl/component_base.yaml.hpp"  // Y_IGNORE
#endif

USERVER_NAMESPACE_BEGIN

namespace components {

// Putting destructor into a cpp file to force vtable instantiation in only 1 translation unit
RawComponentBase::~RawComponentBase() = default;

yaml_config::Schema RawComponentBase::GetStaticConfigSchema() {
    return yaml_config::impl::SchemaFromString(utils::FindResource("src/components/impl/component_base.yaml"));
}

}  // namespace components

USERVER_NAMESPACE_END
