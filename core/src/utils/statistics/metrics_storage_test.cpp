#include <userver/utils/statistics/metrics_storage.hpp>

#include <atomic>

#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/statistics/storage.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

utils::statistics::MetricTag<std::atomic<int>> kFooMetric{"foo-metric"};

}  // namespace

UTEST(MetricsStorage, Smoke) {
  // The second iteration checks that `MetricStorage` instances are independent
  for (int i = 0; i < 2; ++i) {
    utils::statistics::Storage storage;
    utils::statistics::MetricsStorage metrics_storage;
    const auto statistic_holders = metrics_storage.RegisterIn(storage);

    EXPECT_EQ(storage.GetAsJson({"foo-metric"}).ExtractValue()["foo-metric"],
              formats::json::ValueBuilder{0}.ExtractValue());

    metrics_storage.GetMetric(kFooMetric).store(42);
    EXPECT_EQ(storage.GetAsJson({"foo-metric"}).ExtractValue()["foo-metric"],
              formats::json::ValueBuilder{42}.ExtractValue());
  }
}

USERVER_NAMESPACE_END
