#include <userver/components/statistics_storage.hpp>

#include <userver/components/component_context.hpp>
#include <userver/components/scope.hpp>

#include <userver/yaml_config/merge_schemas.hpp>

#ifndef ARCADIA_ROOT
#include "generated/src/components/statistics_storage.yaml.hpp"  // Y_IGNORE
#endif

USERVER_NAMESPACE_BEGIN

namespace components {

StatisticsStorage::StatisticsStorage(const ComponentConfig&, const ComponentContext&)
    : metrics_storage_(std::make_shared<utils::statistics::MetricsStorage>()),
      metrics_storage_registration_(metrics_storage_->RegisterIn(storage_))
{}

StatisticsStorage::~StatisticsStorage() {
    for (auto& entry : metrics_storage_registration_) {
        entry.Unregister();
    }
}

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
    context.RegisterScope(components::MakeScope(
        [&storage,
         common_prefix = std::move(common_prefix),
         func = std::move(func),
         add_labels = std::move(add_labels)] {
            return storage.RegisterWriter(common_prefix, std::move(func), std::move(add_labels));
        }
    ));
}

}  // namespace utils::statistics

USERVER_NAMESPACE_END
