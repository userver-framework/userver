#pragma once

#include <chrono>

#include <userver/rcu/rcu_map.hpp>
#include <userver/utils/span.hpp>
#include <userver/utils/statistics/fwd.hpp>
#include <userver/utils/statistics/histogram.hpp>
#include <userver/utils/statistics/histogram_aggregator.hpp>
#include <userver/utils/statistics/rate_counter.hpp>
#include <userver/ydb/query.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb::impl {

extern const utils::span<const double> kDefaultPerDatabaseBounds;
extern const utils::span<const double> kDefaultPerQueryBounds;

struct StatsCounters final {
  explicit StatsCounters(utils::span<const double> histogram_bounds);

  utils::statistics::RateCounter success;
  utils::statistics::RateCounter error;
  utils::statistics::Histogram timings;
  utils::statistics::RateCounter cancelled;
};

void DumpMetric(utils::statistics::Writer& writer, const StatsCounters& stats);

struct StatsAggregator final {
  explicit StatsAggregator(utils::span<const double> histogram_bounds);

  void Add(const StatsCounters& other);

  void Assign(const StatsCounters& other);

  utils::statistics::Rate success;
  utils::statistics::Rate error;
  utils::statistics::HistogramAggregator timings;
  utils::statistics::Rate cancelled;
};

void DumpMetric(utils::statistics::Writer& writer,
                const StatsAggregator& stats);

struct Stats final {
  Stats(utils::span<const double> by_database_histogram_bounds,
        utils::span<const double> by_query_histogram_bounds);

  const std::vector<double> by_database_histogram_bounds;
  const std::vector<double> by_query_histogram_bounds;

  StatsCounters unnamed_queries;
  rcu::RcuMap<std::string, StatsCounters> by_query;
  rcu::RcuMap<std::string, StatsCounters> by_transaction;
};

void DumpMetric(utils::statistics::Writer& writer, const Stats& stats);

}  // namespace ydb::impl

USERVER_NAMESPACE_END
