#include <storages/mongo/stats_serialize.hpp>

#include <initializer_list>
#include <stdexcept>
#include <tuple>

#include <storages/mongo/stats.hpp>
#include <utils/assert.hpp>
#include <utils/statistics/metadata.hpp>
#include <utils/statistics/percentile_format_json.hpp>

namespace storages::mongo::stats {
namespace {

constexpr std::initializer_list<double> kOperationsStatisticsPercentiles = {
    95, 98, 99, 100};

void Dump(const OperationStatisticsItem& item,
          formats::json::ValueBuilder& builder) {
  Counter::ValueType total_errors = 0;
  auto errors_builder = builder["errors"];
  for (size_t i = 1; i < item.counters.size(); ++i) {
    const auto value = item.counters[i].Load();
    if (value) {
      auto type = static_cast<OperationStatisticsItem::ErrorType>(i);
      errors_builder[ToString(type)] = value;
      total_errors += value;
    }
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

template <typename OperationStatistics>
OperationStatisticsItem DumpAndCombineOperation(
    const OperationStatistics& op_stats,
    formats::json::ValueBuilder&& builder) {
  OperationStatisticsItem overall;
  auto op_builder = builder["by-operation"];
  for (const auto& [type, item_agg] : op_stats.items) {
    const auto item = item_agg->GetStatsForPeriod();
    Dump(item, op_builder[ToString(type)]);
    overall.Add(item);
  }
  utils::statistics::SolomonChildrenAreLabelValues(op_builder,
                                                   "mongo_operation");
  utils::statistics::SolomonSkip(op_builder);
  Dump(overall, builder);
  return overall;
}

template <typename OperationStatistics>
OperationStatisticsItem CombineOperation(const OperationStatistics& op_stats) {
  OperationStatisticsItem overall;
  for (const auto& [_, item_agg] : op_stats.items) {
    overall.Add(item_agg->GetStatsForPeriod());
  }
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
                                       formats::json::ValueBuilder&& builder,
                                       Verbosity verbosity) {
  CombinedCollectionStats overall;

  switch (verbosity) {
    case Verbosity::kTerse:
      for (const auto& [_, stats_ptr] : coll_stats.read) {
        overall.read.Add(CombineOperation(*stats_ptr));
      }
      for (const auto& [_, stats_ptr] : coll_stats.write) {
        overall.write.Add(CombineOperation(*stats_ptr));
      }
      break;
    case Verbosity::kFull: {
      formats::json::ValueBuilder rp_builder(formats::json::Type::kObject);
      for (const auto& [read_pref, stats_ptr] : coll_stats.read) {
        auto rp_overall =
            DumpAndCombineOperation(*stats_ptr, rp_builder[read_pref]);
        overall.read.Add(rp_overall);
      }
      utils::statistics::SolomonChildrenAreLabelValues(rp_builder,
                                                       "mongo_read_preference");
      utils::statistics::SolomonSkip(rp_builder);
      builder["by-read-preference"] = std::move(rp_builder);

      formats::json::ValueBuilder wc_builder(formats::json::Type::kObject);
      for (const auto& [write_concern, stats_ptr] : coll_stats.write) {
        auto wc_overall =
            DumpAndCombineOperation(*stats_ptr, wc_builder[write_concern]);
        overall.write.Add(wc_overall);
      }
      utils::statistics::SolomonChildrenAreLabelValues(wc_builder,
                                                       "mongo_write_concern");
      utils::statistics::SolomonSkip(wc_builder);
      builder["by-write-concern"] = std::move(wc_builder);
    }
  }

  Dump(overall, builder);
  return overall;
}

void Dump(const PoolConnectStatistics& conn_stats,
          formats::json::ValueBuilder&& builder) {
  builder["conn-requests"] = conn_stats.requested.Load();
  builder["conn-created"] = conn_stats.created.Load();
  builder["conn-closed"] = conn_stats.closed.Load();
  builder["overloads"] = conn_stats.overload.Load();

  Dump(conn_stats.items[0]->GetStatsForPeriod(), builder["conn-init"]);

  builder["conn-request-timings"] = utils::statistics::PercentileToJson(
      conn_stats.request_timings_agg.GetStatsForPeriod());
  builder["queue-wait-timings"] = utils::statistics::PercentileToJson(
      conn_stats.queue_wait_timings_agg.GetStatsForPeriod());
}

}  // namespace

void PoolStatisticsToJson(const PoolStatistics& pool_stats,
                          formats::json::ValueBuilder& builder,
                          Verbosity verbosity) {
  Dump(*pool_stats.pool, builder["pool"]);

  CombinedCollectionStats pool_overall;
  formats::json::ValueBuilder coll_builder(formats::json::Type::kObject);
  for (const auto& [coll_name, coll_stats] : pool_stats.collections) {
    auto coll_overall =
        DumpAndCombine(*coll_stats, coll_builder[coll_name], verbosity);
    pool_overall.Add(coll_overall);
  }
  utils::statistics::SolomonChildrenAreLabelValues(coll_builder,
                                                   "mongo_collection");
  utils::statistics::SolomonSkip(coll_builder);
  builder["by-collection"] = std::move(coll_builder);
  Dump(pool_overall, builder);
}

}  // namespace storages::mongo::stats
