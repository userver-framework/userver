#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <functional>
#include <string>

#include <userver/rcu/rcu_map.hpp>
#include <userver/storages/mongo/mongo_error.hpp>
#include <userver/tracing/scope_time.hpp>
#include <userver/utils/not_null.hpp>
#include <userver/utils/statistics/percentile.hpp>
#include <userver/utils/statistics/recentperiod.hpp>
#include <userver/utils/statistics/relaxed_counter.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo::stats {

using Counter = utils::statistics::RelaxedCounter<uint64_t>;
using TimingsPercentile =
    utils::statistics::Percentile</*buckets =*/1000, uint32_t,
                                  /*extra_buckets=*/780,
                                  /*extra_bucket_size=*/50>;
using AggregatedTimingsPercentile =
    utils::statistics::RecentPeriod<TimingsPercentile, TimingsPercentile>;
using AggregatedCounter = utils::statistics::RecentPeriod<Counter, uint64_t>;

enum class ErrorType : std::size_t {
  kSuccess,

  kNetwork,
  kClusterUnavailable,
  kBadServerVersion,
  kAuthFailure,
  kBadQueryArgument,
  kDuplicateKey,
  kWriteConcern,
  kServer,
  kOther,

  kCancelled,
  kPoolOverload,

  kErrorTypesCount
};

inline constexpr auto kErrorTypesCount =
    static_cast<std::size_t>(ErrorType::kErrorTypesCount);

struct OperationStatisticsItem final {
  void Account(ErrorType) noexcept;

  void Reset();

  // TODO don't use RecentPeriod for monotonic counters
  std::array<AggregatedCounter, kErrorTypesCount> counters;
  AggregatedTimingsPercentile timings;
};

std::string_view ToString(ErrorType type);

enum class OpType {
  kInvalid,

  kReadMin,
  kCount = kReadMin,
  kCountApprox,
  kFind,
  kAggregate,

  kWriteMin,
  kInsertOne = kWriteMin,
  kInsertMany,
  kReplaceOne,
  kUpdateOne,
  kUpdateMany,
  kDeleteOne,
  kDeleteMany,
  kFindAndModify,
  kFindAndRemove,
  kBulk,
  kDrop,
};

std::string_view ToString(OpType type);

struct OperationKey final {
  bool operator==(const OperationKey& other) const noexcept;

  // We might want to add an optional user-provided label here in the future.
  OpType op_type{OpType::kInvalid};
};

struct CollectionStatistics final {
  rcu::RcuMap<OperationKey, OperationStatisticsItem> items;
};

struct PoolConnectStatistics {
  PoolConnectStatistics();

  Counter requested;
  Counter created;
  Counter closed;
  Counter overload;

  utils::SharedRef<OperationStatisticsItem> ping;

  AggregatedTimingsPercentile request_timings_agg;
  AggregatedTimingsPercentile queue_wait_timings_agg;
};

struct PoolStatistics {
  PoolStatistics() : pool(utils::MakeSharedRef<PoolConnectStatistics>()) {}

  utils::SharedRef<PoolConnectStatistics> pool;
  rcu::RcuMap<std::string, CollectionStatistics> collections;
};

class OperationStopwatch {
 public:
  explicit OperationStopwatch(std::shared_ptr<OperationStatisticsItem>);
  OperationStopwatch(std::shared_ptr<OperationStatisticsItem>,
                     std::string&& label);

  OperationStopwatch(const OperationStopwatch&) = delete;
  OperationStopwatch(OperationStopwatch&&) noexcept = default;
  ~OperationStopwatch();

  void AccountSuccess();
  void AccountError(MongoError::Kind);
  void Discard();

 private:
  void Account(ErrorType) noexcept;

  std::shared_ptr<OperationStatisticsItem> stats_item_;
  tracing::ScopeTime scope_time_;
};

class ConnectionWaitStopwatch {
 public:
  explicit ConnectionWaitStopwatch(std::shared_ptr<PoolConnectStatistics>);
  ~ConnectionWaitStopwatch();

  ConnectionWaitStopwatch(const ConnectionWaitStopwatch&) = delete;
  ConnectionWaitStopwatch(ConnectionWaitStopwatch&&) noexcept = default;

 private:
  std::shared_ptr<PoolConnectStatistics> stats_ptr_;
  tracing::ScopeTime scope_time_;
};

class ConnectionThrottleStopwatch {
 public:
  explicit ConnectionThrottleStopwatch(std::shared_ptr<PoolConnectStatistics>);
  ~ConnectionThrottleStopwatch();

  ConnectionThrottleStopwatch(const ConnectionThrottleStopwatch&) = delete;
  ConnectionThrottleStopwatch(ConnectionThrottleStopwatch&&) noexcept = default;

  void Stop() noexcept;

 private:
  std::shared_ptr<PoolConnectStatistics> stats_ptr_;
  tracing::ScopeTime scope_time_;
};

}  // namespace storages::mongo::stats

USERVER_NAMESPACE_END

template <>
struct std::hash<USERVER_NAMESPACE::storages::mongo::stats::OperationKey> {
  std::size_t operator()(
      USERVER_NAMESPACE::storages::mongo::stats::OperationKey value) const;
};
