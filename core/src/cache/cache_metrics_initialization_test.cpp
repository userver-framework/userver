#include <userver/utest/utest.hpp>

#include <chrono>

#include <components/component_list_test.hpp>
#include <userver/cache/caching_component_base.hpp>
#include <userver/components/component_base.hpp>
#include <userver/components/component_context.hpp>
#include <userver/components/minimal_component_list.hpp>
#include <userver/components/run.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/single_consumer_event.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/statistics/testing.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

engine::SingleConsumerEvent checked_metrics_before_update;

const auto kStaticConfig = tests::MergeYaml(tests::kMinimalStaticConfig, R"(
components_manager:
    components:
        testsuite-support:
        toy-cache:
            update-types: only-full
            update-interval: 1h
        metrics-checker:
)");

class ToyCache final : public components::CachingComponentBase<int> {
public:
    [[maybe_unused]] static constexpr std::string_view kName = "toy-cache";

    ToyCache(const components::ComponentConfig& config, const components::ComponentContext& context)
        : CachingComponentBase(config, context)
    {
        EarlyStartPeriodicUpdates({});
    }

    ~ToyCache() override { EarlyStopPeriodicUpdates(); }

    void Update(
        cache::UpdateType /*type*/,
        const std::chrono::system_clock::time_point& /*last_update*/,
        const std::chrono::system_clock::time_point& /*now*/,
        cache::UpdateStatisticsScope& stats_scope
    ) override {
        // Wait until the checker component checks metrics at least once.
        const bool wait_succeeded = checked_metrics_before_update.WaitForEventFor(utest::kMaxTestWaitTime);
        EXPECT_TRUE(wait_succeeded) << "Timed out waiting for metrics checker";

        // Complete successfully with size 10.
        Set(10);
        stats_scope.Finish(10);
    }
};

class MetricsChecker final : public components::ComponentBase {
public:
    [[maybe_unused]] static constexpr std::string_view kName = "metrics-checker";

    MetricsChecker(const components::ComponentConfig& config, const components::ComponentContext& context)
        : ComponentBase(config, context)
    {
        auto& stats_storage = context.FindComponent<components::StatisticsStorage>();
        const auto& storage = stats_storage.GetStorage();

        bool cache_age_seen = false;
        bool cache_size_seen = false;

        const auto deadline = engine::Deadline::FromDuration(utest::kMaxTestWaitTime);

        // Check metrics periodically every 10 milliseconds.
        while (!deadline.IsReached()) {
            engine::SleepFor(std::chrono::milliseconds(10));

            const utils::statistics::Snapshot snapshot{storage};

            // Check cache age metric.
            const auto age_metric = snapshot.SingleMetricOptional(
                "cache.any.time.time-from-last-update-start-ms",
                {{"cache_name", "toy-cache"}}
            );
            if (age_metric) {
                const auto age_ms = age_metric->AsInt();
                EXPECT_GE(age_ms, 0) << "Cache age should be non-negative";
                EXPECT_LT(age_ms, 10000) << "Cache age should be less than 10 seconds";
                cache_age_seen = true;
            }

            // Check cache size metric.
            const auto size_metric =
                snapshot.SingleMetricOptional("cache.current-documents-count", {{"cache_name", "toy-cache"}});
            if (size_metric) {
                const auto size = size_metric->AsInt();
                EXPECT_EQ(size, 10) << "Cache size should be 10";
                cache_size_seen = true;
            }

            // Signal to the cache that we checked the metrics state.
            checked_metrics_before_update.Send();

            // If both metrics are set, we're done.
            if (cache_age_seen && cache_size_seen) {
                break;
            }
        }

        // Verify that both metrics were eventually seen.
        EXPECT_TRUE(cache_age_seen) << "Cache age metric should be written after initialization";
        EXPECT_TRUE(cache_size_seen) << "Cache size metric should be written after initialization";
    }
};

class CacheMetricsInitializationTest : public ComponentList {};

}  // namespace

TEST_F(CacheMetricsInitializationTest, MetricsNotWrittenBeforeInitialization) {
    checked_metrics_before_update.Reset();

    components::RunOnce(
        components::InMemoryConfig{kStaticConfig},
        components::MinimalComponentList()
            .Append<components::TestsuiteSupport>()
            .Append<ToyCache>()
            .Append<MetricsChecker>()
    );
}

USERVER_NAMESPACE_END
