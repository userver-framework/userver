#pragma once

#include <userver/cache/lru_map.hpp>
#include <userver/concurrent/mpsc_queue.hpp>
#include <userver/concurrent/variable.hpp>
#include <userver/engine/shared_mutex.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/formats/json_fwd.hpp>
#include <userver/rcu/rcu.hpp>
#include <userver/storages/postgres/options.hpp>
#include <userver/storages/postgres/statistics.hpp>
#include <userver/utils/statistics/rate_counter.hpp>
#include <userver/utils/statistics/recentperiod.hpp>

#include <unordered_map>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::detail {

class StatementStatsStorage final {
 public:
  using Percentile = postgres::Percentile;
  using RateCounter = USERVER_NAMESPACE::utils::statistics::RateCounter;

  enum class ExecutionResult { kSuccess, kError };

  StatementStatsStorage(const StatementMetricsSettings& settings);
  ~StatementStatsStorage();

  void Account(const std::string& statement_name, std::size_t duration_ms,
               ExecutionResult result) const;

  std::unordered_map<std::string, StatementStatistics> GetStatementsStats()
      const;

  void SetSettings(const StatementMetricsSettings& settings);

  // For testing purposes, don't use directly
  void WaitForExhaustion() const;

 private:
  struct StatementEvent final {
    StatementEvent(const std::string& statement_name_, size_t duration_,
                   ExecutionResult result_)
        : statement_name{statement_name_},
          duration_ms{duration_},
          result{result_} {}

    std::string statement_name;
    std::size_t duration_ms;
    ExecutionResult result;
  };
  using EventPtr = std::unique_ptr<StatementEvent>;

  using RecentPeriod =
      USERVER_NAMESPACE::utils::statistics::RecentPeriod<Percentile,
                                                         Percentile>;

  struct StoredStatementStats final {
    RecentPeriod timings{};
    RateCounter executed{};
    RateCounter errors{};
  };

  using Queue = USERVER_NAMESPACE::concurrent::MpscQueue<EventPtr>;

  using StorageType =
      USERVER_NAMESPACE::cache::LruMap<std::string,
                                       std::unique_ptr<StoredStatementStats>>;
  using Storage =
      USERVER_NAMESPACE::concurrent::Variable<StorageType, engine::SharedMutex>;

  struct StorageData final {
    std::unique_ptr<Storage> stats;
    std::shared_ptr<Queue> events_queue;

    engine::TaskWithResult<void> consumer_task;
    Queue::Producer ensure_lifetime_producer;
  };

  bool IsEnabled() const;

  void ProcessEvents();
  void AccountEvent(EventPtr event_ptr) const;
  static StorageData CreateStorageData(size_t max_map_size,
                                       size_t max_queue_size);

  rcu::Variable<StatementMetricsSettings> settings_;
  std::atomic_bool enabled_;

  StorageData data_;
};

}  // namespace storages::postgres::detail

USERVER_NAMESPACE_END
