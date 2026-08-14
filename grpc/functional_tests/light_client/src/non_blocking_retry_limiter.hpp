#pragma once

#include <memory>
#include <string_view>

#include <userver/components/component_base.hpp>
#include <userver/components/component_context.hpp>
#include <userver/utest/using_namespace_userver.hpp>
#include <userver/utils/statistics/metrics_storage_fwd.hpp>

#include <userver/ugrpc/client/retry_limiter.hpp>

namespace functional_tests {

/// A retry-limiter with NO dependency on components::DynamicConfig (unlike
/// BlockingRetryLimiterComponent), used as the explicit `retry-limiter`
/// override for a light client factory. It cannot be `blocking-retry-limiter`
/// itself: that one depends on components::DynamicConfig::GetSource(),
/// which is exactly what would deadlock if a light factory that config-pusher
/// depends on ALSO depended (via an explicit retry-limiter override) on
/// blocking-retry-limiter, and thus transitively on config-pusher's own output.
///
/// Counts CreateRetryLimiter calls via a metric, observable via testsuite,
/// proving that a light factory's explicit retry-limiter override is still
/// honored even though grpc-client-common's shared default is skipped for it.
class NonBlockingRetryLimiterComponent final
    : public ugrpc::client::RetryLimiterFactory,
      public components::ComponentBase {
public:
    static constexpr std::string_view kName = "non-blocking-retry-limiter";

    NonBlockingRetryLimiterComponent(
        const components::ComponentConfig& config,
        const components::ComponentContext& context
    );

    std::unique_ptr<ugrpc::client::RetryLimiter> CreateRetryLimiter(ugrpc::client::RetryLimiterSettings&& settings
    ) const override;

private:
    utils::statistics::MetricsStoragePtr metrics_;
};

}  // namespace functional_tests
