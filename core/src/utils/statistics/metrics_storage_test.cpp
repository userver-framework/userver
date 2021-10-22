#include <userver/utils/statistics/metrics_storage.hpp>

#include <atomic>

#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

utils::statistics::MetricTag<std::atomic<int>> kFooMetric{"foo-metric"};

}  // namespace

TEST(MetricsStorage, Smoke) {
  // The second iteration checks that `MetricStorage` instances are independent
  for (int i = 0; i < 2; ++i) {
    utils::statistics::MetricsStorage storage;
    EXPECT_EQ(storage.DumpMetrics("foo-metric").ExtractValue()["foo-metric"],
              formats::json::ValueBuilder{0}.ExtractValue());

    storage.GetMetric(kFooMetric).store(42);
    EXPECT_EQ(storage.DumpMetrics("foo-metric").ExtractValue()["foo-metric"],
              formats::json::ValueBuilder{42}.ExtractValue());
  }
}

USERVER_NAMESPACE_END
