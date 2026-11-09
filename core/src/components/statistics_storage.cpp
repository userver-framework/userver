#include <userver/components/statistics_storage.hpp>

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#ifndef ARCADIA_ROOT
#include "generated/src/components/statistics_storage.yaml.hpp"  // Y_IGNORE
#endif

USERVER_NAMESPACE_BEGIN

namespace components {

StatisticsStorage::StatisticsStorage(const ComponentConfig& config, const ComponentContext& context)
    : metrics_storage_(std::make_shared<utils::statistics::MetricsStorage>(
          config["captured-metric-tags"].As<std::optional<std::vector<std::string>>>()
      ))
{
    storage_.RegisterWriter(context.Scopes(), "", [&metrics = *metrics_storage_](utils::statistics::Writer& writer) {
        writer = metrics;
    });
}

StatisticsStorage::~StatisticsStorage() = default;

void StatisticsStorage::OnAllComponentsLoaded() { storage_.StopRegisteringExtenders(); }

yaml_config::Schema StatisticsStorage::GetStaticConfigSchema() {
    return yaml_config::MergeSchemasFromResource<RawComponentBase>("src/components/statistics_storage.yaml");
}

}  // namespace components

namespace utils::statistics {

void RegisterWriterScope(
    const components::ComponentContext& context,
    std::string common_prefix,
    WriterFunc func,
    std::vector<Label> add_labels
)
{
    auto& storage = context.FindComponent<components::StatisticsStorage>().GetStorage();
    storage.RegisterWriter(context.Scopes(), std::move(common_prefix), std::move(func), std::move(add_labels));
}

}  // namespace utils::statistics

USERVER_NAMESPACE_END
