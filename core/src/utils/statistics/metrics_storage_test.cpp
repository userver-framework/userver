#include <userver/utils/statistics/metrics_storage.hpp>

#include <atomic>

#include <userver/utest/utest.hpp>
#include <userver/utils/statistics/rate_counter.hpp>
#include <userver/utils/statistics/storage.hpp>
#include <userver/utils/statistics/testing.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

utils::statistics::MetricTag<std::atomic<int>> kFooMetric{"foo-metric"};

utils::statistics::MetricTag<std::atomic<int>> kSampleMetric{"sample-metrics.total"};

struct MetricWritable {
    std::atomic<int> a{1};
    std::atomic<int> b{2};
};

void DumpMetric(utils::statistics::Writer& writer, const MetricWritable& m) {
    writer["a"] = m.a;
    writer["b"] = m.b;
}

utils::statistics::MetricTag<MetricWritable> kWriterMetric{"writer-metric"};

}  // namespace

/// [unit testing metrics]
inline const utils::statistics::MetricTag<utils::statistics::RateCounter> kMetricV1{"a.metric.v1"};

UTEST(MetricsStorage, SampleTesting) {
    utils::statistics::MetricsStorage metrics_storage;

    // Changing metric in test
    metrics_storage.GetMetric(kMetricV1) = utils::statistics::Rate{42};

    // Retrieving metric value
    EXPECT_EQ(metrics_storage.GetMetric(kMetricV1).Load(), utils::statistics::Rate{42});
}
/// [unit testing metrics]

UTEST(MetricsStorage, Smoke) {
    // The second iteration checks that `MetricStorage` instances are independent
    for (int i = 0; i < 2; ++i) {
        utils::statistics::MetricsStorage metrics_storage;

        const utils::statistics::Snapshot snap1{metrics_storage};
        EXPECT_EQ(snap1.SingleMetric("foo-metric").AsInt(), 0);

        metrics_storage.GetMetric(kFooMetric).store(42);
        const utils::statistics::Snapshot snap2{metrics_storage};
        EXPECT_EQ(snap2.SingleMetric("foo-metric").AsInt(), 42);
    }
}

/// [metrics storage Snapshot require_prefix]
UTEST(MetricsStorage, SnapshotRequirePrefix) {
    utils::statistics::MetricsStorage metrics_storage;

    // Fill the metric after creating MetricsStorage and before taking the Snapshot
    metrics_storage.GetMetric(kSampleMetric).store(42);

    // MetricsStorage instantiates all MetricTag instances registered in the
    // binary. require_prefix filters out the unrelated ones.
    const utils::statistics::Snapshot snapshot{
        metrics_storage,
        utils::statistics::SnapshotBuildOptions{.require_prefix = "sample-metrics"},
    };
    // With the default sliced=true the prefix is removed from paths:
    EXPECT_EQ(snapshot.SingleMetric("total").AsInt(), 42);
}
/// [metrics storage Snapshot require_prefix]

UTEST(MetricsStorage, SmokeWriter) {
    // The second iteration checks that `MetricStorage` instances are independent
    for (int i = 0; i < 2; ++i) {
        utils::statistics::MetricsStorage metrics_storage;

        const utils::statistics::Snapshot snap1{metrics_storage};
        EXPECT_EQ(snap1.SingleMetric("writer-metric.a").AsInt(), 1);
        EXPECT_EQ(snap1.SingleMetric("writer-metric.b").AsInt(), 2);
    }
}

UTEST(MetricsStorage, ScopedRegistrationInStorage) {
    utils::statistics::Storage storage;
    const auto metrics_storage = utils::statistics::MakeBoundMetricsStorage(storage, std::nullopt);

    const utils::statistics::Snapshot snap1{storage};
    EXPECT_EQ(snap1.SingleMetric("foo-metric").AsInt(), 0);
    EXPECT_EQ(snap1.SingleMetric("writer-metric.a").AsInt(), 1);
    EXPECT_EQ(snap1.SingleMetric("writer-metric.b").AsInt(), 2);

    metrics_storage->GetMetric(kFooMetric).store(42);
    const utils::statistics::Snapshot snap2{storage};
    EXPECT_EQ(snap2.SingleMetric("foo-metric").AsInt(), 42);
}

UTEST(MetricsStorage, AllowedMetricPaths) {
    {
        utils::statistics::MetricsStorage metrics_storage;
        EXPECT_NO_THROW(metrics_storage.GetMetric(kFooMetric));
    }
    {
        std::vector<std::string> allowed_metric_paths{kWriterMetric.GetPath()};
        utils::statistics::MetricsStorage metrics_storage{allowed_metric_paths};
        EXPECT_THROW(metrics_storage.GetMetric(kFooMetric), std::out_of_range);
    }
}

USERVER_NAMESPACE_END
