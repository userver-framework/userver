#include <userver/cache/caching_component_base.hpp>

#include <userver/dump/dumper.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#ifndef ARCADIA_ROOT
#include "generated/src/cache/caching_component_base.yaml.hpp"  // Y_IGNORE
#endif

USERVER_NAMESPACE_BEGIN

namespace components::impl {

yaml_config::Schema GetCachingComponentBaseSchema() {
    return yaml_config::MergeSchemasFromResource<dump::Dumper>("src/cache/caching_component_base.yaml");
}

}  // namespace components::impl

USERVER_NAMESPACE_END
