#include "blocking_retry_limiter.hpp"

#include <userver/components/statistics_storage.hpp>
#include <userver/dynamic_config/value.hpp>
#include <userver/utils/statistics/metric_tag.hpp>
#include <userver/utils/statistics/metrics_storage.hpp>
#include <userver/utils/statistics/rate_counter.hpp>

namespace functional_tests {

namespace {

// A dedicated dynamic config key for this toy service, used only to prove
// that BlockingRetryLimiterComponent genuinely reads dynamic configs (rather
// than merely depending on components::DynamicConfig for show).
const dynamic_config::Key<bool> kRetryLimiterAllowRetries{"LIGHT_CLIENT_RETRY_LIMITER_ALLOW_RETRIES", true};

// Separate counters (rather than one metric with labels, for simplicity):
// each is incremented at most once, exactly when CreateRetryLimiter is
// actually invoked for the corresponding client. Together they let testsuite
// tell apart which client factories ever consult this
// grpc-client-common-level default retry-limiter.
const utils::statistics::MetricTag<utils::statistics::RateCounter> kCreateCountForRegularClient{
    "blocking-retry-limiter.create-count.regular-greeter-client"
};
const utils::statistics::MetricTag<utils::statistics::RateCounter> kCreateCountForLightClient{
    "blocking-retry-limiter.create-count.light-greeter-client"
};
const utils::statistics::MetricTag<utils::statistics::RateCounter> kCreateCountForNoRetryLimiterClient{
    "blocking-retry-limiter.create-count.regular-greeter-client-no-retry-limiter"
};

class BlockingRetryLimiter final : public ugrpc::client::RetryLimiter {
public:
    explicit BlockingRetryLimiter(dynamic_config::Source config_source)
        : config_source_(config_source)
    {}

    void AccountCompletion(const ugrpc::client::CompletionStatus& /*completion_status*/) override {}

    bool CanRetry() const override {
        const auto snapshot = config_source_.GetSnapshot();
        return snapshot[kRetryLimiterAllowRetries];
    }

private:
    dynamic_config::Source config_source_;
};

}  // namespace

BlockingRetryLimiterComponent::BlockingRetryLimiterComponent(
    const components::ComponentConfig& config,
    const components::ComponentContext& context
)
    : components::ComponentBase(config, context),
      config_source_(context.FindComponent<components::DynamicConfig>().GetSource()),
      metrics_(context.FindComponent<components::StatisticsStorage>().GetMetricsStorage())
{}

std::unique_ptr<ugrpc::client::RetryLimiter> BlockingRetryLimiterComponent::CreateRetryLimiter(
    ugrpc::client::RetryLimiterSettings&& settings
) const {
    if (settings.destination_prefix_in_metrics == "client(regular-greeter-client)") {
        ++metrics_->GetMetric(kCreateCountForRegularClient);
    } else if (settings.destination_prefix_in_metrics == "client(light-greeter-client)") {
        ++metrics_->GetMetric(kCreateCountForLightClient);
    } else if (settings.destination_prefix_in_metrics == "client(regular-greeter-client-no-retry-limiter)") {
        ++metrics_->GetMetric(kCreateCountForNoRetryLimiterClient);
    }
    return std::make_unique<BlockingRetryLimiter>(config_source_);
}

}  // namespace functional_tests
