#include <components/statistics_storage.hpp>

namespace components {

StatisticsStorage::StatisticsStorage(
    const ComponentConfig& config, const components::ComponentContext& context)
    : LoggableComponentBase(config, context) {
  statistics_holder_ = storage_.RegisterExtender(
      "", [this](const auto& request) { return ExtendStatistics(request); });
}

StatisticsStorage::~StatisticsStorage() { statistics_holder_.Unregister(); }

void StatisticsStorage::OnAllComponentsLoaded() {
  storage_.StopRegisteringExtenders();
}

formats::json::ValueBuilder StatisticsStorage::ExtendStatistics(
    const utils::statistics::StatisticsRequest&) {
  return metrics_storage_.DumpMetrics();
}

}  // namespace components
