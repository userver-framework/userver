#include <storages/mongo/stats.hpp>

#include <stdexcept>
#include <tuple>

#include <utils/assert.hpp>
#include <utils/statistics/percentile_format_json.hpp>

namespace storages::mongo::stats {
namespace {

const char* ToCString(OperationStatisticsItem::ErrorType type) {
  using Type = OperationStatisticsItem::ErrorType;
  switch (type) {
    case Type::kSuccess:
      return "success";
    case Type::kNetwork:
      return "network";
    case Type::kClusterUnavailable:
      return "cluster-unavailable";
    case Type::kBadServerVersion:
      return "server-version";
    case Type::kAuthFailure:
      return "auth-failure";
    case Type::kBadQueryArgument:
      return "bad-query-arg";
    case Type::kDuplicateKey:
      return "duplicate-key";
    case Type::kWriteConcern:
      return "write-concern";
    case Type::kServer:
      return "server-error";
    case Type::kOther:
      return "other";

    case Type::kErrorTypesCount:
      UASSERT_MSG(false, "invalid type");
      throw std::logic_error("invalid type");
  }
}

const char* ToCString(ReadOperationStatistics::OpType type) {
  using Type = ReadOperationStatistics::OpType;
  switch (type) {
    case Type::kCount:
      return "count";
    case Type::kCountApprox:
      return "count-approx";
    case Type::kFind:
      return "find";
      // case Type::kGetmore:
      //   return "getmore";

    case Type::kOpTypesCount:
      UASSERT_MSG(false, "invalid type");
      throw std::logic_error("invalid type");
  }
}

const char* ToCString(WriteOperationStatistics::OpType type) {
  using Type = WriteOperationStatistics::OpType;
  switch (type) {
    case Type::kInsertOne:
      return "insert-one";
    case Type::kInsertMany:
      return "insert-many";
    case Type::kReplaceOne:
      return "replace-one";
    case Type::kUpdateOne:
      return "update-one";
    case Type::kUpdateMany:
      return "update-many";
    case Type::kDeleteOne:
      return "delete-one";
    case Type::kDeleteMany:
      return "delete-many";
    case Type::kFindAndModify:
      return "find-and-modify";
    case Type::kFindAndRemove:
      return "find-and-remove";
    case Type::kBulk:
      return "bulk";

    case Type::kOpTypesCount:
      UASSERT_MSG(false, "invalid type");
      throw std::logic_error("invalid type");
  }
}

void Dump(const OperationStatisticsItem& item,
          formats::json::ValueBuilder& builder) {
  Counter::ValueType total_errors = 0;
  auto errors_builder = builder["by-error"];
  for (size_t i = 1; i < item.counters.size(); ++i) {
    auto type = static_cast<OperationStatisticsItem::ErrorType>(i);
    errors_builder[ToCString(type)] = item.counters[i].Load();
    total_errors += item.counters[i];
  }
  builder["success"] = item.counters[0].Load();
  builder["errors"] = total_errors;
  builder["timings"] = utils::statistics::PercentileToJson(item.timings);
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
    Dump(read_stats.items[i], op_builder[ToCString(type)]);
    overall.Add(read_stats.items[i]);
  }
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
    Dump(write_stats.items[i], op_builder[ToCString(type)]);
    overall.Add(write_stats.items[i]);
  }
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
  builder["by-read-preference"] = std::move(rp_builder);

  formats::json::ValueBuilder wc_builder(formats::json::Type::kObject);
  for (const auto& [write_concern, stats_agg] : coll_stats.write) {
    const auto& write_stats = stats_agg->GetStatsForPeriod();
    auto wc_overall = DumpAndCombine(write_stats, wc_builder[write_concern]);
    overall.write.Add(wc_overall);
  }
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
  builder["by-collection"] = std::move(coll_builder);
  Dump(pool_overall, builder);
}

}  // namespace storages::mongo::stats
