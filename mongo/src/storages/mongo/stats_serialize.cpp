#include <storages/mongo/stats.hpp>

#include <initializer_list>
#include <stdexcept>
#include <tuple>

#include <utils/assert.hpp>
#include <utils/statistics/metadata.hpp>
#include <utils/statistics/percentile_format_json.hpp>

namespace storages::mongo::stats {
namespace {

const std::initializer_list<double> kOperationsStatisticsPercentiles = {
    95, 98, 99, 100};

void Dump(const OperationStatisticsItem& item,
          formats::json::ValueBuilder& builder) {
  Counter::ValueType total_errors = 0;
  auto errors_builder = builder["errors"];
  for (size_t i = 1; i < item.counters.size(); ++i) {
    auto type = static_cast<OperationStatisticsItem::ErrorType>(i);
    errors_builder[ToString(type)] = item.counters[i].Load();
    total_errors += item.counters[i];
  }
  utils::statistics::SolomonChildrenAreLabelValues(errors_builder,
                                                   "mongo_error");
  builder["errors"]["total"] = total_errors;
  builder["success"] = item.counters[0].Load();
  builder["timings"] = utils::statistics::PercentileToJson(
      item.timings, kOperationsStatisticsPercentiles);
}

void Dump(const OperationStatisticsItem& item,
          formats::json::ValueBuilder&& builder) {
  Dump(item, builder);
}

OperationStatisticsItem DumpAndCombine(
    const ReadOperationStatistics& read_stats,
    formats::json::ValueBuilder&& builder) {
  OperationStatisticsItem overall;
  auto op_builder = builder["by-operation"];
  for (size_t i = 0; i < read_stats.items.size(); ++i) {
    auto type = static_cast<ReadOperationStatistics::OpType>(i);
    Dump(read_stats.items[i], op_builder[ToString(type)]);
    overall.Add(read_stats.items[i]);
  }
  utils::statistics::SolomonChildrenAreLabelValues(op_builder,
                                                   "mongo_operation");
  utils::statistics::SolomonSkip(op_builder);
  Dump(overall, builder);
  return overall;
}

OperationStatisticsItem DumpAndCombine(
    const WriteOperationStatistics& write_stats,
    formats::json::ValueBuilder&& builder) {
  OperationStatisticsItem overall;
  auto op_builder = builder["by-operation"];
  for (size_t i = 0; i < write_stats.items.size(); ++i) {
    auto type = static_cast<WriteOperationStatistics::OpType>(i);
    Dump(write_stats.items[i], op_builder[ToString(type)]);
    overall.Add(write_stats.items[i]);
  }
  utils::statistics::SolomonChildrenAreLabelValues(op_builder,
                                                   "mongo_operation");
  utils::statistics::SolomonSkip(op_builder);
  Dump(overall, builder);
  return overall;
}

struct CombinedCollectionStats {
  OperationStatisticsItem read;
  OperationStatisticsItem write;

  void Add(const CombinedCollectionStats& other) {
    read.Add(other.read);
    write.Add(other.write);
  }
};

void Dump(const CombinedCollectionStats& combined_stats,
          formats::json::ValueBuilder& builder) {
  Dump(combined_stats.read, builder["read"]);
  Dump(combined_stats.write, builder["write"]);
}

CombinedCollectionStats DumpAndCombine(const CollectionStatistics& coll_stats,
                                       formats::json::ValueBuilder&& builder) {
  CombinedCollectionStats overall;

  formats::json::ValueBuilder rp_builder(formats::json::Type::kObject);
  for (const auto& [read_pref, stats_agg] : coll_stats.read) {
    const auto& read_stats = stats_agg->GetStatsForPeriod();
    auto rp_overall = DumpAndCombine(read_stats, rp_builder[read_pref]);
    overall.read.Add(rp_overall);
  }
  utils::statistics::SolomonChildrenAreLabelValues(rp_builder,
                                                   "mongo_read_preference");
  utils::statistics::SolomonSkip(rp_builder);
  builder["by-read-preference"] = std::move(rp_builder);

  formats::json::ValueBuilder wc_builder(formats::json::Type::kObject);
  for (const auto& [write_concern, stats_agg] : coll_stats.write) {
    const auto& write_stats = stats_agg->GetStatsForPeriod();
    auto wc_overall = DumpAndCombine(write_stats, wc_builder[write_concern]);
    overall.write.Add(wc_overall);
  }
  utils::statistics::SolomonChildrenAreLabelValues(wc_builder,
                                                   "mongo_write_concern");
  utils::statistics::SolomonSkip(wc_builder);
  builder["by-write-concern"] = std::move(wc_builder);

  Dump(overall, builder);
  return overall;
}

void Dump(const PoolConnectStatistics& conn_stats,
          formats::json::ValueBuilder&& builder) {
  builder["conn-requests"] = conn_stats.requested.Load();
  builder["conn-created"] = conn_stats.created.Load();
  builder["conn-closed"] = conn_stats.closed.Load();
  builder["overloads"] = conn_stats.overload.Load();

  Dump(conn_stats.items[0], builder["conn-init"]);

  builder["conn-request-timings"] =
      utils::statistics::PercentileToJson(conn_stats.request_timings);
  builder["queue-wait-timings"] =
      utils::statistics::PercentileToJson(conn_stats.queue_wait_timings);
}

}  // namespace

void PoolStatisticsToJson(const PoolStatistics& pool_stats,
                          formats::json::ValueBuilder& builder) {
  Dump(pool_stats.pool->GetStatsForPeriod(), builder["pool"]);

  CombinedCollectionStats pool_overall;
  formats::json::ValueBuilder coll_builder(formats::json::Type::kObject);
  for (const auto& [coll_name, coll_stats] : pool_stats.collections) {
    auto coll_overall = DumpAndCombine(*coll_stats, coll_builder[coll_name]);
    pool_overall.Add(coll_overall);
  }
  utils::statistics::SolomonChildrenAreLabelValues(coll_builder,
                                                   "mongo_collection");
  utils::statistics::SolomonSkip(coll_builder);
  builder["by-collection"] = std::move(coll_builder);
  Dump(pool_overall, builder);
}

}  // namespace storages::mongo::stats
