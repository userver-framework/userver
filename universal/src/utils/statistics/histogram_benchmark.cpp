#include <userver/utils/statistics/histogram.hpp>

#include <cstddef>
#include <cstdint>
#include <ranges>
#include <string>
#include <utility>
#include <vector>

#include <benchmark/benchmark.h>

#include <userver/utils/algo.hpp>
#include <userver/utils/rand.hpp>
#include <userver/utils/statistics/json.hpp>
#include <userver/utils/statistics/labels.hpp>
#include <userver/utils/statistics/rate.hpp>
#include <userver/utils/statistics/writer.hpp>
#include <utils/gbench_auxiliary.hpp>

USERVER_NAMESPACE_BEGIN

static void HistogramAccount(benchmark::State& state) {
    const auto size = state.range(0);
    const auto iota = utils::impl::AsContainerViaInsert<std::vector<double>>(std::views::iota(std::int64_t{1}, size + 1)
    );

    const auto bounds = Launder(iota);
    auto values_raw = std::vector<double>(1024);
    for (auto& value : values_raw) {
        value = utils::RandRange(0.0, size + 1.0);
    }
    const auto values = Launder(std::move(values_raw));

    utils::statistics::Histogram histogram{bounds};

    while (state.KeepRunningBatch(values.size())) {
        for (const auto value : values) {
            histogram.Account(value);
        }
    }
}

// The range of tested bucket counts must include 20. At some point during
// development, histograms with specifically 20 buckets started performing
// poorly (fixed).
BENCHMARK(HistogramAccount)->DenseRange(10, 50, 10);

namespace {

constexpr std::size_t kMetricCountArgument = 0;
constexpr std::size_t kLabelCountArgument = 1;
constexpr std::size_t kPathCountArgument = 2;
constexpr std::size_t kDuplicateLabelsArgument = 3;
constexpr std::size_t kHistogramBucketCountArgument = 4;
constexpr std::int64_t kGaugeValue = 42;
constexpr std::uint64_t kRateValue = 42;

template <typename Metric>
void StatisticsJson(benchmark::State& state, const Metric& value) {
    const auto metric_count = state.range(kMetricCountArgument);
    const auto label_count = state.range(kLabelCountArgument);
    const auto path_count = state.range(kPathCountArgument);
    const bool duplicate_labels = state.range(kDuplicateLabelsArgument);
    std::vector<std::string> paths;
    for (std::int64_t index = 0; index < path_count; ++index) {
        paths.push_back(fmt::format("metric.{}", index));
    }
    std::vector<utils::statistics::Label> labels;
    for (std::int64_t index = 0; index < label_count; ++index) {
        labels.emplace_back(fmt::format("label.{}", duplicate_labels ? 0 : index), std::to_string(index));
    }
    std::vector<utils::statistics::LabelView> label_views;
    for (const auto& label : labels) {
        label_views.emplace_back(label);
    }

    const auto write_metrics = [&](utils::statistics::Writer& writer) {
        for (std::int64_t index = 0; index < metric_count; ++index) {
            writer[paths[index % path_count]].ValueWithLabels(value, label_views);
        }
    };
    for (auto _ : state) {
        benchmark::DoNotOptimize(utils::statistics::ToJsonFormat(write_metrics));
    }
    state.SetItemsProcessed(state.iterations() * metric_count);
}

void StatisticsJsonGauge(benchmark::State& state) { StatisticsJson(state, kGaugeValue); }

void StatisticsJsonRate(benchmark::State& state) { StatisticsJson(state, utils::statistics::Rate{kRateValue}); }

void StatisticsJsonHistogram(benchmark::State& state) {
    const auto bucket_count = state.range(kHistogramBucketCountArgument);
    std::vector<double> bounds;
    for (std::int64_t index = 0; index < bucket_count; ++index) {
        bounds.push_back(static_cast<double>(index + 1));
    }
    utils::statistics::Histogram histogram{bounds};
    for (const auto bound : bounds) {
        histogram.Account(bound);
    }
    histogram.Account(static_cast<double>(bucket_count + 1));
    StatisticsJson(state, histogram.GetView());
}

constexpr auto kStatisticsJsonArguments = [](auto* benchmark) {
    benchmark->ArgNames({"metrics", "labels", "paths", "duplicate_labels"});
    benchmark->Args({1, 0, 1, 0});
    benchmark->Args({128, 0, 128, 0});
    benchmark->Args({128, 8, 128, 0});
    benchmark->Args({128, 8, 1, 0});
    benchmark->Args({128, 8, 2, 0});
    benchmark->Args({128, 17, 1, 0});
    benchmark->Args({128, 17, 1, 1});
};

}  // namespace

BENCHMARK(StatisticsJsonGauge)->Apply(kStatisticsJsonArguments);
BENCHMARK(StatisticsJsonRate)->Apply(kStatisticsJsonArguments);
BENCHMARK(StatisticsJsonHistogram)
    ->ArgNames({"metrics", "labels", "paths", "duplicate_labels", "buckets"})
    ->Args({1, 0, 1, 0, 0})
    ->Args({1, 0, 1, 0, 1})
    ->Args({128, 8, 1, 0, 16})
    ->Args({128, 8, 1, 0, 17})
    ->Args({128, 8, 2, 0, 50})
    ->Args({128, 8, 128, 0, 128});

USERVER_NAMESPACE_END
