#include <userver/components/statistics_storage.hpp>

#include <userver/yaml_config/merge_schemas.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

StatisticsStorage::StatisticsStorage(const ComponentConfig& config,
                                     const ComponentContext& context)
    : LoggableComponentBase(config, context),
      metrics_storage_(std::make_shared<utils::statistics::MetricsStorage>()),
      metrics_storage_registration_(metrics_storage_->RegisterIn(storage_)) {}

StatisticsStorage::~StatisticsStorage() = default;

void StatisticsStorage::OnAllComponentsLoaded() {
  storage_.StopRegisteringExtenders();
}

yaml_config::Schema StatisticsStorage::GetStaticConfigSchema() {
  yaml_config::Schema schema(R"(
type: object
description: statistics-storage config
additionalProperties: false
properties: {}
)");
  yaml_config::Merge(schema, LoggableComponentBase::GetStaticConfigSchema());
  return schema;
}

}  // namespace components

USERVER_NAMESPACE_END
