#pragma once

#include <userver/components/component_base.hpp>
#include <userver/components/component_context.hpp>
#include <userver/dynamic_config/storage/component.hpp>
#include <userver/utest/using_namespace_userver.hpp>
#include <userver/utils/statistics/metrics_storage_fwd.hpp>

#include <userver/ugrpc/client/retry_limiter.hpp>

namespace functional_tests {

/// A toy stand-in for the standard `statistics-retry-limiter` component: it
/// depends on a real (non-constant) `components::DynamicConfig` source, just
/// like the real `grpc-statistics-retry-limiter` library does via
/// `components::StatisticsReachClient`. Used here to demonstrate that this
/// dependency is not eagerly pulled into "light" gRPC client factories.
///
/// Also exposes a metric counting `CreateRetryLimiter` calls per client (see
/// CreateRetryLimiter), observable via testsuite, proving whether this
/// grpc-client-common-level default retry-limiter is (or isn't) ever
/// consulted for a given client factory.
class BlockingRetryLimiterComponent final
    : public ugrpc::client::RetryLimiterFactory,
      public components::ComponentBase {
public:
    static constexpr std::string_view kName = "blocking-retry-limiter";

    BlockingRetryLimiterComponent(
        const components::ComponentConfig& config,
        const components::ComponentContext& context
    );

    std::unique_ptr<ugrpc::client::RetryLimiter> CreateRetryLimiter(ugrpc::client::RetryLimiterSettings&& settings
    ) const override;

private:
    dynamic_config::Source config_source_;
    utils::statistics::MetricsStoragePtr metrics_;
};

}  // namespace functional_tests
