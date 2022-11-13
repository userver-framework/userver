#pragma once

#include <array>
#include <atomic>
#include <chrono>
#include <map>
#include <mutex>
#include <unordered_map>

#include <userver/concurrent/variable.hpp>
#include <userver/storages/redis/impl/command.hpp>
#include <userver/utils/statistics/aggregated_values.hpp>
#include <userver/utils/statistics/percentile.hpp>
#include <userver/utils/statistics/recentperiod.hpp>

#include "redis_state.hpp"

USERVER_NAMESPACE_BEGIN

namespace redis {

std::chrono::milliseconds MillisecondsSinceEpoch();

constexpr size_t ReplySizeBucketCount = 15;
constexpr size_t RequestSizeBucketCount = 15;
constexpr size_t TimingBucketCount = 15;

class Statistics {
 public:
  Statistics() = default;

  void AccountStateChanged(RedisState new_state);
  void AccountCommandSent(const CommandPtr& cmd);
  void AccountReplyReceived(const ReplyPtr& reply, const CommandPtr& cmd);
  void AccountPing(std::chrono::milliseconds ping);
  void AccountError(int code);

  using Percentile = utils::statistics::Percentile<2048>;
  using RecentPeriod =
      utils::statistics::RecentPeriod<Percentile, Percentile,
                                      utils::datetime::SteadyClock>;
  using CommandTimings = std::unordered_map<std::string, RecentPeriod>;

  std::atomic<RedisState> state{RedisState::kInit};
  std::atomic_llong reconnects{0};
  std::atomic<std::chrono::milliseconds> session_start_time{};
  RecentPeriod request_size_percentile;
  RecentPeriod reply_size_percentile;
  RecentPeriod timings_percentile;
  concurrent::Variable<CommandTimings, std::mutex> command_timings_percentile;
  std::atomic_llong last_ping_ms{};

  std::array<std::atomic_llong, REDIS_ERR_MAX + 1> error_count{{}};
};

struct InstanceStatistics {
  InstanceStatistics(const Statistics& other)
      : state(other.state.load(std::memory_order_relaxed)),
        reconnects(other.reconnects.load(std::memory_order_relaxed)),
        session_start_time(
            other.session_start_time.load(std::memory_order_relaxed)),
        request_size_percentile(
            other.request_size_percentile.GetStatsForPeriod()),
        reply_size_percentile(other.reply_size_percentile.GetStatsForPeriod()),
        timings_percentile(other.timings_percentile.GetStatsForPeriod()),
        last_ping_ms(other.last_ping_ms.load(std::memory_order_relaxed)) {
    for (size_t i = 0; i < error_count.size(); i++)
      error_count[i] = other.error_count[i].load(std::memory_order_relaxed);
    auto command_timings = other.command_timings_percentile.Lock();
    for (const auto& [command, timings] : *command_timings)
      command_timings_percentile.emplace(command, timings.GetStatsForPeriod());
  }

  InstanceStatistics() : InstanceStatistics(Statistics()) {}

  void Add(const InstanceStatistics& other) {
    reconnects += other.reconnects;
    request_size_percentile.Add(other.request_size_percentile);
    reply_size_percentile.Add(other.reply_size_percentile);
    timings_percentile.Add(other.timings_percentile);

    for (size_t i = 0; i < error_count.size(); i++)
      error_count[i] += other.error_count[i];

    for (const auto& [command, timings] : other.command_timings_percentile)
      command_timings_percentile[command].Add(timings);
  }

  RedisState state;
  long long reconnects;
  std::chrono::milliseconds session_start_time;
  Statistics::Percentile request_size_percentile;
  Statistics::Percentile reply_size_percentile;
  Statistics::Percentile timings_percentile;
  std::unordered_map<std::string, Statistics::Percentile>
      command_timings_percentile;
  long long last_ping_ms;

  std::array<long long, REDIS_ERR_MAX + 1> error_count{{}};
};

struct ShardStatistics {
  InstanceStatistics GetShardTotalStatistics() const;

  std::map<std::string, InstanceStatistics> instances;
  bool is_ready = false;
  std::chrono::steady_clock::time_point last_ready_time;
};

struct SentinelStatisticsInternal {
  SentinelStatisticsInternal() = default;
  SentinelStatisticsInternal(const SentinelStatisticsInternal& other)
      : redis_not_ready(other.redis_not_ready.load(std::memory_order_relaxed)) {
  }

  std::atomic_llong redis_not_ready{0};
};

struct SentinelStatistics {
  explicit SentinelStatistics(const SentinelStatisticsInternal internal)
      : internal(internal) {}

  InstanceStatistics GetShardGroupTotalStatistics() const;

  ShardStatistics sentinel;
  std::map<std::string, ShardStatistics> masters;
  std::map<std::string, ShardStatistics> slaves;
  SentinelStatisticsInternal internal;
};

}  // namespace redis

USERVER_NAMESPACE_END
