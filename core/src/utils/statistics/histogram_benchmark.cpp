#include <userver/utils/statistics/histogram.hpp>

#include <benchmark/benchmark.h>
#include <boost/range/irange.hpp>

#include <userver/utils/algo.hpp>
#include <userver/utils/rand.hpp>
#include <utils/gbench_auxilary.hpp>

USERVER_NAMESPACE_BEGIN

void HistogramAccount(benchmark::State& state) {
  const auto size = state.range(0);
  const auto iota = utils::AsContainer<std::vector<double>>(
      boost::irange(std::int64_t{1}, size + 1));

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

USERVER_NAMESPACE_END
