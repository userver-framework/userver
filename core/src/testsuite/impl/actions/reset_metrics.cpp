#include "reset_metrics.hpp"

#include <userver/components/component.hpp>
#include <userver/components/statistics_storage.hpp>

USERVER_NAMESPACE_BEGIN

namespace testsuite::impl::actions {

ResetMetrics::ResetMetrics(
    const components::ComponentContext& component_context)
    : metrics_storage_(
          component_context.FindComponent<components::StatisticsStorage>()
              .GetMetricsStorage()) {}

formats::json::Value ResetMetrics::Perform(const formats::json::Value&) const {
  metrics_storage_->ResetMetrics();
  return {};
}

}  // namespace testsuite::impl::actions

USERVER_NAMESPACE_END
