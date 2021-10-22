#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <functional>
#include <ratio>
#include <string>

#include <userver/rcu/rcu_map.hpp>
#include <userver/storages/mongo/mongo_error.hpp>
#include <userver/utils/prof.hpp>
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

template <typename T>
using Aggregator = utils::statistics::RecentPeriod<T, T>;

struct OperationStatisticsItem {
  enum ErrorType {
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

    kErrorTypesCount
  };

  OperationStatisticsItem() = default;
  OperationStatisticsItem(const OperationStatisticsItem&);

  template <typename Rep = int64_t, typename Period = std::ratio<1>>
  void Add(const OperationStatisticsItem& other,
           std::chrono::duration<Rep, Period> curr_duration = {},
           std::chrono::duration<Rep, Period> past_duration = {});

  void Reset();

  std::array<Counter, kErrorTypesCount> counters;
  TimingsPercentile timings;
};

std::string ToString(OperationStatisticsItem::ErrorType type);

struct ReadOperationStatistics {
  enum OpType {
    kCount,
    kCountApprox,
    kFind,
    kGetMore,
  };

  rcu::RcuMap<OpType, Aggregator<OperationStatisticsItem>> items;
};

std::string ToString(ReadOperationStatistics::OpType type);

struct WriteOperationStatistics {
  enum OpType {
    kInsertOne,
    kInsertMany,
    kReplaceOne,
    kUpdateOne,
    kUpdateMany,
    kDeleteOne,
    kDeleteMany,
    kFindAndModify,
    kFindAndRemove,
    kBulk,
  };

  rcu::RcuMap<OpType, Aggregator<OperationStatisticsItem>> items;
};

std::string ToString(WriteOperationStatistics::OpType type);

struct CollectionStatistics {
  // read preference -> stats
  rcu::RcuMap<std::string, ReadOperationStatistics> read;

  // write concern -> stats
  rcu::RcuMap<std::string, WriteOperationStatistics> write;
};

struct PoolConnectStatistics {
  // for OperationStopwatch compatibility
  enum OpType {
    kPing,

    kOpTypesCount
  };

  PoolConnectStatistics();

  Counter requested;
  Counter created;
  Counter closed;
  Counter overload;

  std::array<std::shared_ptr<Aggregator<OperationStatisticsItem>>,
             OpType::kOpTypesCount>
      items;

  Aggregator<TimingsPercentile> request_timings_agg;
  Aggregator<TimingsPercentile> queue_wait_timings_agg;
};

std::string ToString(PoolConnectStatistics::OpType type);

struct PoolStatistics {
  PoolStatistics() : pool(std::make_shared<PoolConnectStatistics>()) {}

  std::shared_ptr<PoolConnectStatistics> pool;
  rcu::RcuMap<std::string, CollectionStatistics> collections;
};

template <typename OperationStatistics>
class OperationStopwatch {
 public:
  OperationStopwatch();
  OperationStopwatch(const std::shared_ptr<OperationStatistics>&,
                     typename OperationStatistics::OpType);
  ~OperationStopwatch();

  OperationStopwatch(const OperationStopwatch&) = delete;
  OperationStopwatch(OperationStopwatch&&) noexcept = default;

  void Reset(const std::shared_ptr<OperationStatistics>&,
             typename OperationStatistics::OpType);

  void AccountSuccess();
  void AccountError(MongoError::Kind);
  void Discard();

 private:
  void Account(OperationStatisticsItem::ErrorType) noexcept;

  std::shared_ptr<Aggregator<OperationStatisticsItem>> stats_item_agg_;
  ScopeTime scope_time_;
};

class ConnectionWaitStopwatch {
 public:
  explicit ConnectionWaitStopwatch(std::shared_ptr<PoolConnectStatistics>);
  ~ConnectionWaitStopwatch();

  ConnectionWaitStopwatch(const ConnectionWaitStopwatch&) = delete;
  ConnectionWaitStopwatch(ConnectionWaitStopwatch&&) noexcept = default;

 private:
  std::shared_ptr<PoolConnectStatistics> stats_ptr_;
  ScopeTime scope_time_;
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
  ScopeTime scope_time_;
};

}  // namespace storages::mongo::stats

USERVER_NAMESPACE_END

// Have to be defined at this point
namespace std {

template <>
struct hash<USERVER_NAMESPACE::storages::mongo::stats::ReadOperationStatistics::
                OpType> {
  size_t operator()(
      USERVER_NAMESPACE::storages::mongo::stats::ReadOperationStatistics::OpType
          type) const {
    return hash<int>()(static_cast<int>(type));
  }
};

template <>
struct hash<USERVER_NAMESPACE::storages::mongo::stats::
                WriteOperationStatistics::OpType> {
  size_t operator()(USERVER_NAMESPACE::storages::mongo::stats::
                        WriteOperationStatistics::OpType type) const {
    return hash<int>()(static_cast<int>(type));
  }
};

}  // namespace std

USERVER_NAMESPACE_BEGIN

namespace storages::mongo::stats {

template <typename Rep, typename Period>
void OperationStatisticsItem::Add(
    const OperationStatisticsItem& other,
    std::chrono::duration<Rep, Period> curr_duration,
    std::chrono::duration<Rep, Period> past_duration) {
  for (size_t i = 0; i < counters.size(); ++i) {
    counters[i] += other.counters[i];
  }
  timings.Add(other.timings, curr_duration, past_duration);
}

}  // namespace storages::mongo::stats

USERVER_NAMESPACE_END
