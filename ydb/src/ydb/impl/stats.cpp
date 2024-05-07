#include "stats.hpp"

#include <userver/utils/algo.hpp>
#include <userver/utils/statistics/writer.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb::impl {

namespace {

// +20% steps up until 1000ms, then ~x2 steps up until 100000ms.
// Reason: most "sane" DB requests take <100ms.
// Requests that require full DB scan may take minutes, but that's not
// recommended or typical usage.
// Actual inaccuracy will be around +-5% on pracice due to interpolation.
constexpr double kDefaultPerDatabaseBoundsArray[] = {
    1,    2,    3,    5,    7,     10,    13,    16,    20,    24,
    29,   35,   42,   50,   60,    71,    84,    100,   120,   144,
    173,  208,  250,  300,  360,   430,   520,   620,   730,   850,
    1000, 1800, 3200, 5600, 10000, 18000, 32000, 56000, 100000};

// "Golden test". Note that there is 1 more "inf" bucket.
static_assert(std::size(kDefaultPerDatabaseBoundsArray) == 39);

// A reduced set of buckets for by-query metrics.
// We try to spare the service's metrics quota by default.
constexpr double kDefaultPerQueryBoundsArray[] = {
    5, 10, 20, 35, 60, 100, 173, 300, 520, 1000, 3200, 10000, 32000, 100000};

// "Golden test". Note that there is 1 more "inf" bucket.
static_assert(std::size(kDefaultPerQueryBoundsArray) == 14);

utils::statistics::Rate Load(const utils::statistics::Rate& metric) {
  return metric;
}

utils::statistics::Rate Load(const utils::statistics::RateCounter& metric) {
  return metric.Load();
}

template <typename T>
void DoDumpMetric(utils::statistics::Writer& writer, const T& stats) {
  const auto success = Load(stats.success);
  const auto error = Load(stats.error);
  writer["total"] = success + error;
  writer["success"] = success;
  writer["error"] = error;

  writer["timings"] = stats.timings;

  writer["cancelled"] = stats.cancelled;
}

}  // namespace

constexpr utils::span<const double> kDefaultPerDatabaseBounds{
    kDefaultPerDatabaseBoundsArray};
constexpr utils::span<const double> kDefaultPerQueryBounds{
    kDefaultPerQueryBoundsArray};

StatsCounters::StatsCounters(utils::span<const double> histogram_bounds)
    : timings(histogram_bounds) {}

void DumpMetric(utils::statistics::Writer& writer, const StatsCounters& stats) {
  DoDumpMetric(writer, stats);
}

StatsAggregator::StatsAggregator(utils::span<const double> histogram_bounds)
    : timings(histogram_bounds) {}

void StatsAggregator::Add(const StatsCounters& other) {
  success += other.success.Load();
  error += other.error.Load();
  timings.Add(other.timings.GetView());
  cancelled += other.cancelled.Load();
}

void StatsAggregator::Assign(const StatsCounters& other) {
  success = other.success.Load();
  error = other.error.Load();
  timings.Reset();
  timings.Add(other.timings.GetView());
  cancelled = other.cancelled.Load();
}

void DumpMetric(utils::statistics::Writer& writer,
                const StatsAggregator& stats) {
  DoDumpMetric(writer, stats);
}

Stats::Stats(utils::span<const double> by_database_histogram_bounds,
             utils::span<const double> by_query_histogram_bounds)
    : by_database_histogram_bounds(utils::AsContainer<std::vector<double>>(
          by_database_histogram_bounds)),
      by_query_histogram_bounds(
          utils::AsContainer<std::vector<double>>(by_query_histogram_bounds)),
      unnamed_queries(by_database_histogram_bounds) {}

void DumpMetric(utils::statistics::Writer& writer, const Stats& stats) {
  {
    StatsAggregator current_approx{stats.by_query_histogram_bounds};
    StatsAggregator total{stats.by_database_histogram_bounds};

    total.Add(stats.unnamed_queries);

    {
      auto by_query_writer = writer["by-query"];

      current_approx.Assign(stats.unnamed_queries);
      by_query_writer.ValueWithLabels(current_approx, {"ydb_query", "UNNAMED"});

      for (const auto& [query_name, query_stats] : stats.by_query) {
        total.Add(*query_stats);

        current_approx.Assign(*query_stats);
        by_query_writer.ValueWithLabels(current_approx,
                                        {"ydb_query", query_name});
      }
    }

    writer["queries-total"] = total;
  }

  {
    StatsAggregator current_approx{stats.by_query_histogram_bounds};
    StatsAggregator total{stats.by_database_histogram_bounds};

    {
      auto by_transaction_writer = writer["by-transaction"];

      for (const auto& [tx_name, tx_stats] : stats.by_transaction) {
        total.Add(*tx_stats);

        current_approx.Assign(*tx_stats);
        by_transaction_writer.ValueWithLabels(current_approx,
                                              {"ydb_transaction", tx_name});
      }
    }

    writer["transactions-total"] = total;
  }
}

}  // namespace ydb::impl

USERVER_NAMESPACE_END
