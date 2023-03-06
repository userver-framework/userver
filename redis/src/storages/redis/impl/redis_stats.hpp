#pragma once

#include <array>
#include <atomic>
#include <chrono>
#include <map>
#include <string_view>
#include <unordered_map>

#include <userver/storages/redis/impl/base.hpp>
#include <userver/storages/redis/impl/redis_state.hpp>
#include <userver/storages/redis/impl/types.hpp>
#include <userver/utils/statistics/aggregated_values.hpp>
#include <userver/utils/statistics/percentile.hpp>
#include <userver/utils/statistics/recentperiod.hpp>

#include <storages/redis/impl/reply_status_strings.hpp>

USERVER_NAMESPACE_BEGIN

namespace redis {

std::chrono::milliseconds MillisecondsSinceEpoch();

constexpr size_t ReplySizeBucketCount = 15;
constexpr size_t RequestSizeBucketCount = 15;
constexpr size_t TimingBucketCount = 15;

class Statistics {
 public:
  Statistics();

  void AccountStateChanged(RedisState new_state);
  void AccountCommandSent(const CommandPtr& cmd);
  void AccountReplyReceived(const ReplyPtr& reply, const CommandPtr& cmd);
  void AccountPing(std::chrono::milliseconds ping);
  void AccountError(ReplyStatus code);

  using Percentile = utils::statistics::Percentile<2048>;
  using RecentPeriod =
      utils::statistics::RecentPeriod<Percentile, Percentile,
                                      utils::datetime::SteadyClock>;

  std::atomic<RedisState> state{RedisState::kInit};
  std::atomic_llong reconnects{0};
  std::atomic<std::chrono::milliseconds> session_start_time{};
  RecentPeriod request_size_percentile;
  RecentPeriod reply_size_percentile;
  RecentPeriod timings_percentile;
  std::unordered_map<std::string_view, RecentPeriod> command_timings_percentile;
  std::atomic_llong last_ping_ms{};
  std::atomic_bool is_syncing = false;
  std::atomic_size_t offset_from_master_bytes = 0;

  std::array<std::atomic_llong, kReplyStatusMap.size()> error_count{{}};
};

struct InstanceStatistics {
  InstanceStatistics(const MetricsSettings& settings, const Statistics& other)
      : settings(settings),
        state(other.state.load(std::memory_order_relaxed)),
        reconnects(other.reconnects.load(std::memory_order_relaxed)),
        session_start_time(
            other.session_start_time.load(std::memory_order_relaxed)),
        request_size_percentile(
            other.request_size_percentile.GetStatsForPeriod()),
        reply_size_percentile(other.reply_size_percentile.GetStatsForPeriod()),
        timings_percentile(other.timings_percentile.GetStatsForPeriod()),
        last_ping_ms(other.last_ping_ms.load(std::memory_order_relaxed)),
        is_syncing(other.is_syncing.load(std::memory_order_relaxed)),
        offset_from_master(
            other.offset_from_master_bytes.load(std::memory_order_relaxed)) {
    for (size_t i = 0; i < error_count.size(); i++)
      error_count[i] = other.error_count[i].load(std::memory_order_relaxed);
    for (const auto& [command, timings] : other.command_timings_percentile) {
      auto stats = timings.GetStatsForPeriod();
      if (!stats.Count()) continue;
      command_timings_percentile.emplace(command, std::move(stats));
    }
  }

  InstanceStatistics(const MetricsSettings& settings)
      : InstanceStatistics(settings, Statistics()) {}

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

  const MetricsSettings& settings;
  RedisState state;
  long long reconnects;
  std::chrono::milliseconds session_start_time;
  Statistics::Percentile request_size_percentile;
  Statistics::Percentile reply_size_percentile;
  Statistics::Percentile timings_percentile;
  std::unordered_map<std::string, Statistics::Percentile>
      command_timings_percentile;
  long long last_ping_ms;
  bool is_syncing;
  long long offset_from_master;

  std::array<long long, kReplyStatusMap.size()> error_count{{}};
};

struct ShardStatistics {
  ShardStatistics(const MetricsSettings& settings) : shard_total(settings) {}

  InstanceStatistics shard_total;
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
  SentinelStatistics(const MetricsSettings& settings,
                     const SentinelStatisticsInternal internal)
      : shard_group_total(settings), internal(internal) {}

  InstanceStatistics GetShardGroupTotalStatistics() const;

  std::optional<ShardStatistics> sentinel;
  std::map<std::string, ShardStatistics> masters;
  std::map<std::string, ShardStatistics> slaves;
  InstanceStatistics shard_group_total;
  SentinelStatisticsInternal internal;
};

void DumpMetric(utils::statistics::Writer& writer,
                const InstanceStatistics& stats, bool real_instance = true);

void DumpMetric(utils::statistics::Writer& writer,
                const ShardStatistics& stats);

void DumpMetric(utils::statistics::Writer& writer,
                const SentinelStatistics& stats);

}  // namespace redis

USERVER_NAMESPACE_END
