#include <userver/cache/lru_cache_component_base.hpp>

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/dump/config.hpp>
#include <userver/dump/dumper.hpp>
#include <userver/dynamic_config/storage/component.hpp>
#include <userver/yaml_config/merge_schemas.hpp>
#include <userver/yaml_config/schema.hpp>

#ifndef ARCADIA_ROOT
#include "generated/src/cache/lru_cache_component_base.yaml.hpp"  // Y_IGNORE
#endif

USERVER_NAMESPACE_BEGIN

namespace cache::impl {

void RegisterOnStatisticsStorage(
    const components::ComponentContext& context,
    const std::string& name,
    std::function<void(utils::statistics::Writer&)> func
) {
    utils::statistics::RegisterWriterScope(context, "cache", std::move(func), {{"cache_name", name}});
}

dynamic_config::Source FindDynamicConfigSource(const components::ComponentContext& context) {
    return context.FindComponent<components::DynamicConfig>().GetSource();
}

bool IsDumpSupportEnabled(const components::ComponentConfig& config) {
    const bool dump_support_enabled = config.HasMember(dump::kDump);
    if (dump_support_enabled) {
        const auto min_interval = config[dump::kDump][dump::kMinDumpInterval];
        if (min_interval.IsMissing()) {
            throw std::runtime_error(fmt::format(
                "Missing static config field '{}'. Please fill it in explicitly. A "
                "low value (e.g. the default '0') will typically result in too "
                "frequent dump writes of the LRU cache.",
                min_interval.GetPath()
            ));
        }
    }
    return dump_support_enabled;
}

yaml_config::Schema GetLruCacheComponentBaseSchema() {
    return yaml_config::MergeSchemasFromResource<dump::Dumper>("src/cache/lru_cache_component_base.yaml");
}

}  // namespace cache::impl

USERVER_NAMESPACE_END
