#include <userver/components/statistics_storage.hpp>

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
  return yaml_config::Schema(R"(
type: object
description: statistics-storage config
additionalProperties: false
properties: {}
)");
}

}  // namespace components

USERVER_NAMESPACE_END
