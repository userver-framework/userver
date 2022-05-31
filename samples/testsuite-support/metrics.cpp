#include "metrics.hpp"

#include <userver/components/component_context.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/utils/statistics/metric_tag.hpp>
#include <userver/utils/statistics/metrics_storage.hpp>

namespace tests::handlers {
namespace {

const utils::statistics::MetricTag<std::atomic<int>> kFooMetric{
    "sample-metrics.foo"};

}  // namespace

Metrics::Metrics(const userver::components::ComponentConfig& config,
                 const userver::components::ComponentContext& context)
    : userver::server::handlers::HttpHandlerJsonBase(config, context),
      metrics_(context.FindComponent<components::StatisticsStorage>()
                   .GetMetricsStorage()) {}

formats::json::Value Metrics::HandleRequestJsonThrow(
    [[maybe_unused]] const server::http::HttpRequest& request,
    [[maybe_unused]] const formats::json::Value& request_body,
    [[maybe_unused]] server::request::RequestContext& context) const {
  formats::json::ValueBuilder result;
  metrics_->GetMetric(kFooMetric)++;
  return result.ExtractValue();
}

}  // namespace tests::handlers
