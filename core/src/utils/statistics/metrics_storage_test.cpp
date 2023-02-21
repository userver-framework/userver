#include <userver/utils/statistics/metrics_storage.hpp>

#include <atomic>

#include <userver/utest/utest.hpp>
#include <userver/utils/statistics/storage.hpp>
#include <userver/utils/statistics/testing.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

utils::statistics::MetricTag<std::atomic<int>> kFooMetric{"foo-metric"};

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

UTEST(MetricsStorage, Smoke) {
  using utils::statistics::Request;

  // The second iteration checks that `MetricStorage` instances are independent
  for (int i = 0; i < 2; ++i) {
    utils::statistics::Storage storage;
    utils::statistics::MetricsStorage metrics_storage;
    const auto statistic_holders = metrics_storage.RegisterIn(storage);

    utils::statistics::Snapshot snap1{storage};
    EXPECT_EQ(snap1.SingleMetric("foo-metric").AsInt(), 0);

    metrics_storage.GetMetric(kFooMetric).store(42);
    utils::statistics::Snapshot snap2{storage};
    EXPECT_EQ(snap2.SingleMetric("foo-metric").AsInt(), 42);
  }
}

UTEST(MetricsStorage, SmokeWriter) {
  using utils::statistics::Request;

  // The second iteration checks that `MetricStorage` instances are independent
  for (int i = 0; i < 2; ++i) {
    utils::statistics::Storage storage;
    utils::statistics::MetricsStorage metrics_storage;
    const auto statistic_holders = metrics_storage.RegisterIn(storage);

    utils::statistics::Snapshot snap1{storage};
    EXPECT_EQ(snap1.SingleMetric("writer-metric.a").AsInt(), 1);
    EXPECT_EQ(snap1.SingleMetric("writer-metric.b").AsInt(), 2);
  }
}

USERVER_NAMESPACE_END
