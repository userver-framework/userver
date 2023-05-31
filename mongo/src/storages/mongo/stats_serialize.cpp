#include <storages/mongo/stats_serialize.hpp>

#include <initializer_list>

#include <userver/utils/assert.hpp>
#include <userver/utils/statistics/writer.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo::stats {
namespace {

constexpr std::initializer_list<double> kOperationsStatisticsPercentiles = {
    95, 98, 99, 100};

using Rate = utils::statistics::Rate;

struct OperationStatisticsSum final {
  void Add(const OperationStatisticsItem& other) {
    for (std::size_t i = 0; i < counters.size(); ++i) {
      counters[i] += other.counters[i].Load();
    }
    timings.Add(other.timings.GetStatsForPeriod());
  }

  void Add(const OperationStatisticsSum& other) {
    for (size_t i = 0; i < counters.size(); ++i) {
      counters[i] += other.counters[i];
    }
    timings.Add(other.timings);
  }

  std::array<Rate, kErrorTypesCount> counters{{}};
  TimingsPercentile timings;
};

Rate LoadCounter(const Counter& counter) { return counter.Load(); }

Rate LoadCounter(const Rate& counter) { return counter; }

void WriteOperationTimings(utils::statistics::Writer& writer,
                           const AggregatedTimingsPercentile& counter) {
  DumpMetric(writer, counter.GetStatsForPeriod(),
             kOperationsStatisticsPercentiles);
}

void WriteOperationTimings(utils::statistics::Writer& writer,
                           const TimingsPercentile& counter) {
  DumpMetric(writer, counter, kOperationsStatisticsPercentiles);
}

template <typename OperationStatistics>
void DumpOperationStatistics(utils::statistics::Writer& writer,
                             const OperationStatistics& item) {
  Rate total_errors{0};
  {
    auto errors_writer = writer["errors"];
    for (size_t i = 1; i < item.counters.size(); ++i) {
      const auto value = LoadCounter(item.counters[i]);
      if (value) {
        auto type = static_cast<ErrorType>(i);
        errors_writer.ValueWithLabels(value, {"mongo_error", ToString(type)});
        total_errors += value;
      }
    }
  }
  writer["errors-total"].ValueWithLabels(total_errors,
                                         {"mongo_error", "total"});
  writer["success"] = item.counters[0];
  if (auto timings_writer = writer["timings-1min"]) {
    WriteOperationTimings(timings_writer, item.timings);
  }
}

void DumpMetric(utils::statistics::Writer& writer,
                const OperationStatisticsSum& item) {
  DumpOperationStatistics(writer, item);
}

bool IsRead(const OperationKey& key) {
  return key.op_type >= OpType::kReadMin && key.op_type < OpType::kWriteMin;
}

std::string_view GetDirection(const OperationKey& key) {
  return IsRead(key) ? "read" : "write";
}

struct CombinedCollectionStats {
  OperationStatisticsSum read;
  OperationStatisticsSum write;

  void Add(const CombinedCollectionStats& other) {
    read.Add(other.read);
    write.Add(other.write);
  }

  OperationStatisticsSum& GetTotalForOperation(const OperationKey& key) {
    return IsRead(key) ? read : write;
  }
};

void DumpMetric(utils::statistics::Writer& writer,
                const CombinedCollectionStats& combined_stats) {
  writer.ValueWithLabels(combined_stats.read, {"mongo_direction", "read"});
  writer.ValueWithLabels(combined_stats.write, {"mongo_direction", "write"});
}

struct FormattedCollectionStatistics final {
  const CollectionStatistics& stats;
  StatsVerbosity verbosity;
  CombinedCollectionStats& overall_stats;
};

void DumpMetric(utils::statistics::Writer& writer,
                const FormattedCollectionStatistics& coll_stats) {
  CombinedCollectionStats coll_overall;

  {
    auto by_operation_writer = writer["by-operation"];

    for (const auto& [stats_key, stats_ptr] : coll_stats.stats.items) {
      if (coll_stats.verbosity == StatsVerbosity::kFull) {
        by_operation_writer.ValueWithLabels(
            *stats_ptr, {{"mongo_operation", ToString(stats_key.op_type)},
                         {"mongo_direction", GetDirection(stats_key)}});
      }

      coll_overall.GetTotalForOperation(stats_key).Add(*stats_ptr);
    }
  }

  writer["by-collection"] = coll_overall;

  coll_stats.overall_stats.Add(coll_overall);
}

}  // namespace

void DumpMetric(utils::statistics::Writer& writer,
                const OperationStatisticsItem& item) {
  DumpOperationStatistics(writer, item);
}

void DumpMetric(utils::statistics::Writer& writer,
                const PoolConnectStatistics& conn_stats) {
  writer["conn-requests"] = conn_stats.requested;
  writer["conn-created"] = conn_stats.created;
  writer["conn-closed"] = conn_stats.closed;
  writer["overloads"] = conn_stats.overload;

  writer["conn-init"] = *conn_stats.ping;

  writer["conn-request-timings-1min"] = conn_stats.request_timings_agg;
  writer["queue-wait-timings-1min"] = conn_stats.queue_wait_timings_agg;
}

void DumpMetric(utils::statistics::Writer& writer,
                const PoolStatistics& pool_stats, StatsVerbosity verbosity) {
  writer["pool"] = *pool_stats.pool;
  writer["congestion-control"] = pool_stats.congestion_control;

  CombinedCollectionStats pool_overall;
  for (const auto& [coll_name, coll_stats] : pool_stats.collections) {
    UASSERT(coll_stats);
    writer.ValueWithLabels(
        FormattedCollectionStatistics{*coll_stats, verbosity, pool_overall},
        {"mongo_collection", coll_name});
  }

  writer["by-database"] = pool_overall;
}

}  // namespace storages::mongo::stats

USERVER_NAMESPACE_END
