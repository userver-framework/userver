#include "non_blocking_retry_limiter.hpp"

#include <userver/components/statistics_storage.hpp>
#include <userver/utils/statistics/metric_tag.hpp>
#include <userver/utils/statistics/metrics_storage.hpp>
#include <userver/utils/statistics/rate_counter.hpp>

namespace functional_tests {

namespace {

const utils::statistics::MetricTag<utils::statistics::RateCounter> kCreateCountForLightOverrideClient{
    "non-blocking-retry-limiter.create-count.light-greeter-client-explicit-retry-limiter"
};

class NonBlockingRetryLimiter final : public ugrpc::client::RetryLimiter {
public:
    void AccountCompletion(const ugrpc::client::CompletionStatus& /*completion_status*/) override {}

    bool CanRetry() const override { return true; }
};

}  // namespace

NonBlockingRetryLimiterComponent::NonBlockingRetryLimiterComponent(
    const components::ComponentConfig& config,
    const components::ComponentContext& context
)
    : components::ComponentBase(config, context),
      metrics_(context.FindComponent<components::StatisticsStorage>().GetMetricsStorage())
{}

std::unique_ptr<ugrpc::client::RetryLimiter> NonBlockingRetryLimiterComponent::CreateRetryLimiter(
    ugrpc::client::RetryLimiterSettings&& /*settings*/
) const {
    ++metrics_->GetMetric(kCreateCountForLightOverrideClient);
    return std::make_unique<NonBlockingRetryLimiter>();
}

}  // namespace functional_tests
