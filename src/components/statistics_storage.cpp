#include <components/statistics_storage.hpp>

namespace components {

StatisticsStorage::StatisticsStorage(
    const ComponentConfig& config, const components::ComponentContext& context)
    : LoggableComponentBase(config, context) {}

StatisticsStorage::~StatisticsStorage() = default;

}  // namespace components
