#include <userver/components/statistics_storage.hpp>

#include <userver/yaml_config/merge_schemas.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

StatisticsStorage::StatisticsStorage(const ComponentConfig&,
                                     const ComponentContext&)
    : metrics_storage_(std::make_shared<utils::statistics::MetricsStorage>()),
      metrics_storage_registration_(metrics_storage_->RegisterIn(storage_)) {}

StatisticsStorage::~StatisticsStorage() {
  for (auto& entry : metrics_storage_registration_) {
    entry.Unregister();
  }
}

void StatisticsStorage::OnAllComponentsLoaded() {
  storage_.StopRegisteringExtenders();
}

yaml_config::Schema StatisticsStorage::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<impl::ComponentBase>(R"(
type: object
description: Component that keeps a utils::statistics::Storage storage for metrics.
additionalProperties: false
properties: {}
)");
}

}  // namespace components

USERVER_NAMESPACE_END
