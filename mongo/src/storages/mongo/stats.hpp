#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <ratio>
#include <string>

#include <rcu/rcu_map.hpp>
#include <storages/mongo/mongo_error.hpp>
#include <utils/statistics/percentile.hpp>
#include <utils/statistics/recentperiod.hpp>
#include <utils/statistics/relaxed_counter.hpp>

namespace storages::mongo::stats {

using Counter = utils::statistics::RelaxedCounter<uint32_t>;
using TimingsPercentile =
    utils::statistics::Percentile</*buckets =*/1000, uint32_t,
                                  /*extra_buckets=*/900,
                                  /*extra_bucket_size=*/10>;

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

  template <typename Rep = int64_t, typename Period = std::ratio<1>>
  void Add(const OperationStatisticsItem& other,
           std::chrono::duration<Rep, Period> curr_duration = {},
           std::chrono::duration<Rep, Period> past_duration = {});

  void Reset();

  std::array<Counter, kErrorTypesCount> counters;
  TimingsPercentile timings;
};

struct ReadOperationStatistics {
  enum OpType {
    kCount,
    kCountApprox,
    kFind,
    kGetMore,

    kOpTypesCount
  };

  template <typename Rep = int64_t, typename Period = std::ratio<1>>
  void Add(const ReadOperationStatistics& other,
           std::chrono::duration<Rep, Period> curr_duration = {},
           std::chrono::duration<Rep, Period> past_duration = {});

  void Reset();

  std::array<OperationStatisticsItem, kOpTypesCount> items;
};

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

    kOpTypesCount
  };

  template <typename Rep = int64_t, typename Period = std::ratio<1>>
  void Add(const WriteOperationStatistics& other,
           std::chrono::duration<Rep, Period> curr_duration = {},
           std::chrono::duration<Rep, Period> past_duration = {});

  void Reset();

  std::array<OperationStatisticsItem, kOpTypesCount> items;
};

struct CollectionStatistics {
  // read preference -> stats
  rcu::RcuMap<std::string, Aggregator<ReadOperationStatistics>> read;

  // write concern -> stats
  rcu::RcuMap<std::string, Aggregator<WriteOperationStatistics>> write;
};

struct PoolConnectStatistics {
  // for OperationStopwatch compatibility
  enum OpType {
    kPing,

    kOpTypesCount
  };

  template <typename Rep = int64_t, typename Period = std::ratio<1>>
  void Add(const PoolConnectStatistics& other,
           std::chrono::duration<Rep, Period> curr_duration = {},
           std::chrono::duration<Rep, Period> past_duration = {});

  void Reset();

  Counter requested;
  Counter created;
  Counter closed;
  Counter overload;

  std::array<OperationStatisticsItem, kOpTypesCount> items;

  TimingsPercentile request_timings;
  TimingsPercentile queue_wait_timings;
};

struct PoolStatistics {
  PoolStatistics()
      : pool(std::make_shared<Aggregator<PoolConnectStatistics>>()) {}

  std::shared_ptr<Aggregator<PoolConnectStatistics>> pool;
  rcu::RcuMap<std::string, CollectionStatistics> collections;
};

class StopwatchImpl {
 public:
  StopwatchImpl() noexcept;

  uint32_t ElapsedMs() const noexcept;

 private:
  std::chrono::steady_clock::time_point start_;
};

template <typename OperationStatistics>
class OperationStopwatch {
 public:
  OperationStopwatch() = default;
  OperationStopwatch(std::shared_ptr<Aggregator<OperationStatistics>>,
                     typename OperationStatistics::OpType);
  ~OperationStopwatch();

  OperationStopwatch(const OperationStopwatch&) = delete;
  OperationStopwatch(OperationStopwatch&&) noexcept = default;
  OperationStopwatch& operator=(const OperationStopwatch&) = delete;
  OperationStopwatch& operator=(OperationStopwatch&&) noexcept = default;

  void AccountSuccess();
  void AccountError(MongoError::Kind);
  void Discard();

 private:
  void Account(OperationStatisticsItem::ErrorType) noexcept;

  std::shared_ptr<Aggregator<OperationStatistics>> stats_agg_;
  StopwatchImpl stopwatch_;
  typename OperationStatistics::OpType op_type_;
};

class PoolRequestStopwatch {
 public:
  explicit PoolRequestStopwatch(
      std::shared_ptr<Aggregator<PoolConnectStatistics>>);
  ~PoolRequestStopwatch();

  PoolRequestStopwatch(const PoolRequestStopwatch&) = delete;
  PoolRequestStopwatch(PoolRequestStopwatch&&) noexcept = default;
  PoolRequestStopwatch& operator=(const PoolRequestStopwatch&) = delete;
  PoolRequestStopwatch& operator=(PoolRequestStopwatch&&) noexcept = default;

 private:
  std::shared_ptr<Aggregator<PoolConnectStatistics>> stats_agg_;
  StopwatchImpl stopwatch_;
};

class PoolQueueStopwatch {
 public:
  explicit PoolQueueStopwatch(
      std::shared_ptr<Aggregator<PoolConnectStatistics>>);
  ~PoolQueueStopwatch();

  PoolQueueStopwatch(const PoolQueueStopwatch&) = delete;
  PoolQueueStopwatch(PoolQueueStopwatch&&) noexcept = default;
  PoolQueueStopwatch& operator=(const PoolQueueStopwatch&) = delete;
  PoolQueueStopwatch& operator=(PoolQueueStopwatch&&) noexcept = default;

  void Stop() noexcept;

 private:
  std::shared_ptr<Aggregator<PoolConnectStatistics>> stats_agg_;
  StopwatchImpl stopwatch_;
};

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

template <typename Rep, typename Period>
void ReadOperationStatistics::Add(
    const ReadOperationStatistics& other,
    std::chrono::duration<Rep, Period> curr_duration,
    std::chrono::duration<Rep, Period> past_duration) {
  for (size_t i = 0; i < items.size(); ++i) {
    items[i].Add(other.items[i], curr_duration, past_duration);
  }
}

template <typename Rep, typename Period>
void WriteOperationStatistics::Add(
    const WriteOperationStatistics& other,
    std::chrono::duration<Rep, Period> curr_duration,
    std::chrono::duration<Rep, Period> past_duration) {
  for (size_t i = 0; i < items.size(); ++i) {
    items[i].Add(other.items[i], curr_duration, past_duration);
  }
}

template <typename Rep, typename Period>
void PoolConnectStatistics::Add(
    const PoolConnectStatistics& other,
    std::chrono::duration<Rep, Period> curr_duration,
    std::chrono::duration<Rep, Period> past_duration) {
  requested += other.requested;
  created += other.created;
  closed += other.closed;
  overload += other.overload;
  for (size_t i = 0; i < items.size(); ++i) {
    items[i].Add(other.items[i], curr_duration, past_duration);
  }
  request_timings.Add(other.request_timings, curr_duration, past_duration);
  queue_wait_timings.Add(other.queue_wait_timings, curr_duration,
                         past_duration);
}

}  // namespace storages::mongo::stats
