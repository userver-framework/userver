#include <userver/utils/statistics/histogram.hpp>

#include <limits>

#include <boost/range/irange.hpp>

#include <userver/utest/utest.hpp>
#include <userver/utils/algo.hpp>
#include <userver/utils/enumerate.hpp>
#include <userver/utils/statistics/fmt.hpp>
#include <userver/utils/statistics/graphite.hpp>
#include <userver/utils/statistics/histogram_aggregator.hpp>
#include <userver/utils/statistics/json.hpp>
#include <userver/utils/statistics/metric_tag.hpp>
#include <userver/utils/statistics/metrics_storage.hpp>
#include <userver/utils/statistics/pretty_format.hpp>
#include <userver/utils/statistics/prometheus.hpp>
#include <userver/utils/statistics/solomon.hpp>
#include <userver/utils/statistics/storage.hpp>
#include <userver/utils/statistics/testing.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

auto Bounds() { return std::vector<double>{1.5, 5, 42, 60}; }

void AccountSome(utils::statistics::Histogram& histogram) {
  histogram.Account(10);
  histogram.Account(1.2);
  histogram.Account(1.8);
  histogram.Account(100);
  histogram.Account(30, 4);
}

UTEST(StatisticsHistogram, Account) {
  utils::statistics::Histogram histogram{Bounds()};
  AccountSome(histogram);

  const auto view = histogram.GetView();
  EXPECT_EQ(view.GetBucketCount(), 4);
  EXPECT_EQ(view.GetValueAtInf(), 1);

  EXPECT_EQ(view.GetUpperBoundAt(0), 1.5);
  EXPECT_EQ(view.GetValueAt(0), 1);
  EXPECT_EQ(view.GetUpperBoundAt(1), 5);
  EXPECT_EQ(view.GetValueAt(1), 1);
  EXPECT_EQ(view.GetUpperBoundAt(2), 42);
  EXPECT_EQ(view.GetValueAt(2), 5);
  EXPECT_EQ(view.GetUpperBoundAt(3), 60);
  EXPECT_EQ(view.GetValueAt(3), 0);
}

UTEST(StatisticsHistogram, ValueOnBucketBorder) {
  utils::statistics::Histogram histogram{Bounds()};
  histogram.Account(5);
  EXPECT_EQ(fmt::to_string(histogram.GetView()),
            "[1.5]=0,[5]=1,[42]=0,[60]=0,[inf]=0");
}

UTEST(StatisticsHistogram, AccountForEachBucketCount) {
  constexpr double kInf = 9001;
  constexpr std::uint64_t kInfCount = 42;

  for (std::size_t i = 1; i < 100; ++i) {
    const auto bounds = boost::irange(std::size_t{1}, i + 1);
    utils::statistics::Histogram histogram{
        utils::AsContainer<std::vector<double>>(bounds)};

    ASSERT_EQ(histogram.GetView().GetBucketCount(), i);
    for (const auto bound : bounds) {
      histogram.Account(bound, bound);
    }
    histogram.Account(kInf, kInfCount);

    const auto view = histogram.GetView();
    for (const auto [idx, bound] : utils::enumerate(bounds)) {
      EXPECT_EQ(view.GetValueAt(idx), bound);
    }
    EXPECT_EQ(view.GetValueAtInf(), kInfCount);
  }
}

UTEST(StatisticsHistogram, Total) {
  utils::statistics::Histogram histogram{Bounds()};
  AccountSome(histogram);
  EXPECT_EQ(fmt::to_string(histogram.GetView()),
            "[1.5]=1,[5]=1,[42]=5,[60]=0,[inf]=1");
  EXPECT_EQ(histogram.GetView().GetTotalCount(), 8);
}

UTEST(StatisticsHistogram, Sample) {
  /// [sample]
  utils::statistics::Storage storage;

  utils::statistics::Histogram histogram{std::vector<double>{1.5, 5, 42, 60}};

  auto statistics_holder = storage.RegisterWriter(
      "test", [&](utils::statistics::Writer& writer) { writer = histogram; });

  histogram.Account(10);
  histogram.Account(1.2);
  histogram.Account(1.8);
  histogram.Account(100);
  histogram.Account(30, 4);  // Account 4 times

  const utils::statistics::Snapshot snapshot{storage};
  EXPECT_EQ(fmt::to_string(snapshot.SingleMetric("test")),
            "[1.5]=1,[5]=1,[42]=5,[60]=0,[inf]=1");
  /// [sample]
}

UTEST(StatisticsHistogram, Copy) {
  utils::statistics::Histogram histogram{Bounds()};
  AccountSome(histogram);

  const utils::statistics::Histogram copy{histogram};
  EXPECT_EQ(fmt::to_string(copy.GetView()),
            "[1.5]=1,[5]=1,[42]=5,[60]=0,[inf]=1");
}

UTEST(StatisticsHistogram, Add) {
  /// [HistogramAggregator]
  utils::statistics::Histogram histogram1{Bounds()};
  AccountSome(histogram1);
  AccountSome(histogram1);

  utils::statistics::Histogram histogram2{Bounds()};
  AccountSome(histogram2);

  utils::statistics::HistogramAggregator aggregator{Bounds()};
  aggregator.Add(histogram1.GetView());
  aggregator.Add(histogram2.GetView());

  EXPECT_EQ(fmt::to_string(aggregator.GetView()),
            "[1.5]=3,[5]=3,[42]=15,[60]=0,[inf]=3");
  /// [HistogramAggregator]
}

UTEST(StatisticsHistogram, Reset) {
  utils::statistics::Histogram histogram{Bounds()};
  AccountSome(histogram);
  ResetMetric(histogram);
  const utils::statistics::Histogram zero_histogram{Bounds()};
  EXPECT_EQ(histogram.GetView(), zero_histogram.GetView());
}

UTEST(StatisticsHistogram, ZeroBuckets) {
  utils::statistics::Histogram histogram{std::vector<double>{}};
  EXPECT_EQ(histogram.GetView().GetBucketCount(), 0);
  EXPECT_EQ(histogram.GetView().GetValueAtInf(), 0);

  AccountSome(histogram);
  EXPECT_EQ(histogram.GetView().GetValueAtInf(), 8);

  utils::statistics::Histogram histogram2{histogram};
  histogram2.Account(42);

  utils::statistics::HistogramAggregator aggregator{std::vector<double>{}};
  aggregator.Add(histogram.GetView());
  aggregator.Add(histogram2.GetView());
  EXPECT_EQ(aggregator.GetView().GetValueAtInf(), 17);

  EXPECT_EQ(fmt::to_string(aggregator.GetView()), "[inf]=17");
}

utils::statistics::Histogram MakeHistogramForNarrowing() {
  utils::statistics::Histogram histogram{
      std::vector<double>{2.0, 4.0, 6.0, 8.0}};
  histogram.Account(1.0, 1);
  histogram.Account(3.0, 10);
  histogram.Account(5.0, 100);
  histogram.Account(7.0, 1000);
  histogram.Account(9.0, 10000);
  return histogram;
}

UTEST(StatisticsHistogram, BucketNarrowing) {
  const auto histogram = MakeHistogramForNarrowing();
  {
    utils::statistics::HistogramAggregator aggregator{
        std::vector<double>{2.0, 8.0}};
    aggregator.Add(histogram.GetView());
    EXPECT_EQ(fmt::to_string(aggregator.GetView()),
              "[2]=1,[8]=1110,[inf]=10000");
  }
  {
    utils::statistics::HistogramAggregator aggregator{
        std::vector<double>{4.0, 6.0}};
    aggregator.Add(histogram.GetView());
    EXPECT_EQ(fmt::to_string(aggregator.GetView()),
              "[4]=11,[6]=100,[inf]=11000");
  }
}

UTEST_DEATH(StatisticsHistogramDeathTest, InvalidBuckets) {
  EXPECT_UINVARIANT_FAILURE_MSG(
      (utils::statistics::Histogram{std::vector<double>{10, 5, 3}}),
      "Histogram bounds must be sorted");
  EXPECT_UINVARIANT_FAILURE_MSG(
      (utils::statistics::Histogram{std::vector<double>{5, 5}}),
      "Histogram bounds must not contain duplicates");
  EXPECT_UINVARIANT_FAILURE_MSG(
      utils::statistics::Histogram{std::vector<double>{-5}},
      "Histogram bounds must be positive");
  EXPECT_UINVARIANT_FAILURE_MSG(
      utils::statistics::Histogram{std::vector<double>{0}},
      "Histogram bounds must be positive");
  EXPECT_UINVARIANT_FAILURE_MSG(
      utils::statistics::Histogram{
          std::vector<double>{std::numeric_limits<double>::quiet_NaN()}},
      "Histogram bounds must be positive");
  EXPECT_UINVARIANT_FAILURE_MSG(
      utils::statistics::Histogram{
          std::vector<double>{std::numeric_limits<double>::infinity()}},
      "Histogram bounds must be positive");
}

class StatisticsHistogramMetricTag : public testing::Test {
 protected:
  StatisticsHistogramMetricTag()
      : metrics_storage_holder_(metrics_storage_.RegisterIn(storage_)) {}

  utils::statistics::Storage& GetStorage() { return storage_; }

  utils::statistics::MetricsStorage& GetMetricsStorage() {
    return metrics_storage_;
  }

 private:
  utils::statistics::Storage storage_;
  utils::statistics::MetricsStorage metrics_storage_;
  std::vector<utils::statistics::Entry> metrics_storage_holder_;
};

void AssertAccounted10(utils::statistics::HistogramView histogram) {
  utils::statistics::Histogram expected{Bounds()};
  expected.Account(10);
  EXPECT_EQ(histogram, expected.GetView());
}

/// [metric tag]
utils::statistics::MetricTag<utils::statistics::Histogram>
    kRuntimeHistogramMetric{"histogram_metric_sample_runtime", Bounds()};

void AccountHistogram(utils::statistics::MetricsStorage& metrics_storage) {
  metrics_storage.GetMetric(kRuntimeHistogramMetric).Account(10);
}
/// [metric tag]

UTEST_F(StatisticsHistogramMetricTag, RuntimeSample) {
  AccountHistogram(GetMetricsStorage());
  AssertAccounted10(
      GetMetricsStorage().GetMetric(kRuntimeHistogramMetric).GetView());
}

class StatisticsHistogramFormat : public testing::Test {
 protected:
  StatisticsHistogramFormat() {
    AccountSome(histogram_);
    statistics_holder_ = storage_.RegisterWriter(
        "test",
        [&](utils::statistics::Writer& writer) { writer = histogram_; });
  }

  const utils::statistics::Storage& GetStorage() const { return storage_; }

 private:
  utils::statistics::Storage storage_;
  utils::statistics::Histogram histogram_{Bounds()};
  utils::statistics::Entry statistics_holder_;
};

using StatisticsHistogramFormatDeathTest = StatisticsHistogramFormat;

UTEST_F(StatisticsHistogramFormat, JsonFormat) {
  EXPECT_EQ(
      formats::json::FromString(utils::statistics::ToJsonFormat(GetStorage())),
      formats::json::FromString(R"(
{
  "test": [
    {
      "value": {
        "bounds": [1.5, 5.0, 42.0, 60.0],
        "buckets": [1, 1, 5, 0],
        "inf": 1
      },
      "labels": {},
      "type": "HIST_RATE"
    }
  ]
}
      )"));
}

UTEST_F(StatisticsHistogramFormat, PrettyFormat) {
  EXPECT_EQ(utils::statistics::ToPrettyFormat(GetStorage()),
            "test:\tHIST_RATE\t[1.5]=1,[5]=1,[42]=5,[60]=0,[inf]=1\n");
}

UTEST_F(StatisticsHistogramFormat, Prometheus) {
  constexpr std::string_view expected = R"(# TYPE test histogram
test_bucket{le="1.5"} 1
test_bucket{le="5"} 2
test_bucket{le="42"} 7
test_bucket{le="60"} 7
test_bucket{le="+Inf"} 8
test_count{} 8
)";
  EXPECT_EQ(utils::statistics::ToPrometheusFormat(GetStorage()), expected);
}

UTEST_F(StatisticsHistogramFormat, PrometheusUntyped) {
  constexpr std::string_view expected = R"(# TYPE test histogram
test_bucket{le="1.5"} 1
test_bucket{le="5"} 2
test_bucket{le="42"} 7
test_bucket{le="60"} 7
test_bucket{le="+Inf"} 8
test_count{} 8
)";
  EXPECT_EQ(utils::statistics::ToPrometheusFormatUntyped(GetStorage()),
            expected);
}

UTEST_F(StatisticsHistogramFormat, CustomLabels) {
  utils::statistics::Storage storage;
  utils::statistics::Histogram histogram{Bounds()};
  AccountSome(histogram);
  auto statistics_holder =
      storage.RegisterWriter("test", [&](utils::statistics::Writer& writer) {
        writer.ValueWithLabels(
            histogram, {{"custom_label_1", "1"}, {"custom_label_2", "2"}});
      });

  constexpr std::string_view expected = R"(# TYPE test histogram
test_bucket{le="1.5",custom_label_1="1",custom_label_2="2"} 1
test_bucket{le="5",custom_label_1="1",custom_label_2="2"} 2
test_bucket{le="42",custom_label_1="1",custom_label_2="2"} 7
test_bucket{le="60",custom_label_1="1",custom_label_2="2"} 7
test_bucket{le="+Inf",custom_label_1="1",custom_label_2="2"} 8
test_count{custom_label_1="1",custom_label_2="2"} 8
)";
  EXPECT_EQ(utils::statistics::ToPrometheusFormat(storage), expected);
}

// TODO support HistogramView in Graphite metrics
UTEST_F_DEATH(StatisticsHistogramFormatDeathTest, Graphite) {
  EXPECT_DEBUG_DEATH(
      EXPECT_EQ(utils::statistics::ToGraphiteFormat(GetStorage()), ""),
      "not supported");
}

UTEST_F(StatisticsHistogramFormat, SolomonFormat) {
  EXPECT_EQ(formats::json::FromString(
                utils::statistics::ToSolomonFormat(GetStorage(), {})),
            formats::json::FromString(R"(
{
  "metrics": [
    {
      "labels": {
        "sensor": "test"
      },
      "hist": {
        "bounds": [1.5, 5.0, 42.0, 60.0],
        "buckets": [1, 1, 5, 0],
        "inf": 1
      },
      "type": "HIST_RATE"
    }
  ]
}
      )"));
}

}  // namespace

USERVER_NAMESPACE_END
