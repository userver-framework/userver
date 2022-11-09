#include "metrics_portability.hpp"

#include <userver/components/component.hpp>
#include <userver/components/statistics_storage.hpp>

#include <userver/utils/statistics/portability_info.hpp>

USERVER_NAMESPACE_BEGIN

namespace testsuite::impl::actions {

MetricsPortability::MetricsPortability(
    const components::ComponentContext& component_context)
    : statistics_storage_(
          component_context.FindComponent<components::StatisticsStorage>()
              .GetStorage()) {}

formats::json::Value MetricsPortability::Perform(
    const formats::json::Value& request_body) const {
  const auto statistics_request = utils::statistics::Request::MakeWithPrefix(
      request_body["prefix"].As<std::string>({}));
  const auto info = utils::statistics::GetPortabilityWarnings(
      statistics_storage_, statistics_request);

  return formats::json::ValueBuilder{info}.ExtractValue();
}

}  // namespace testsuite::impl::actions

USERVER_NAMESPACE_END
