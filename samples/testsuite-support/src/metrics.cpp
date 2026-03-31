#include "metrics.hpp"

#include <userver/components/component_context.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/utils/statistics/metric_tag.hpp>
#include <userver/utils/statistics/metrics_storage.hpp>
#include <userver/utils/statistics/rate_counter.hpp>

namespace tests::handlers {
namespace {

/// [metrics definition]
const utils::statistics::MetricTag<utils::statistics::RateCounter> kFooMetric{"sample-metrics.foo"};
/// [metrics definition]

}  // namespace

Metrics::Metrics(const components::ComponentConfig& config, const components::ComponentContext& context)
    : server::handlers::HttpHandlerJsonBase(config, context),
      metrics_(context.FindComponent<components::StatisticsStorage>().GetMetricsStorage())
{}

formats::json::Value Metrics::HandleRequestJsonThrow(
    [[maybe_unused]] const server::http::HttpRequest& request,
    [[maybe_unused]] const formats::json::Value& request_body,
    [[maybe_unused]] server::request::RequestContext& context
) const {
    request.GetHttpResponse().SetContentType(http::content_type::kApplicationJson);
    formats::json::ValueBuilder result;

    /// [metrics usage]
    utils::statistics::RateCounter& foo_metric = metrics_->GetMetric(kFooMetric);
    ++foo_metric;  // safe to increment concurrently
    /// [metrics usage]

    return result.ExtractValue();
}

}  // namespace tests::handlers
