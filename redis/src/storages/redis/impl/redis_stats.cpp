#include <storages/redis/impl/redis_stats.hpp>

#include <array>

#include <storages/redis/impl/command.hpp>
#include <storages/redis/impl/redis.hpp>
#include <userver/logging/log.hpp>
#include <userver/storages/redis/impl/base.hpp>
#include <userver/storages/redis/impl/reply.hpp>
#include <userver/utils/assert.hpp>

#include <hiredis/hiredis.h>

USERVER_NAMESPACE_BEGIN

namespace redis {

namespace {

const std::string_view kCommandTypes[] = {
    "append",
    "auth",
    "cluster",
    "dbsize",
    "del",
    "eval",
    "evalsha",
    "exists",
    "expire",
    "flushdb",
    "geoadd",
    "georadius_ro",
    "geosearch",
    "get",
    "getset",
    "hdel",
    "hexists",
    "hget",
    "hgetall",
    "hincrby",
    "hincrbyfloat",
    "hkeys",
    "hlen",
    "hmget",
    "hmset",
    "hscan",
    "hset",
    "hsetnx",
    "hvals",
    "incr",
    "info",
    "keys",
    "lindex",
    "llen",
    "lpop",
    "lpush",
    "lpushx",
    "lrange",
    "lrem",
    "ltrim",
    "mget",
    "mset",
    "multi",
    "persist",
    "pexpire",
    "ping",
    "psubscribe",
    "publish",
    "punsubscribe",
    "readonly",
    "rename",
    "rpop",
    "rpush",
    "rpushx",
    "sadd",
    "scan",
    "scard",
    "script",
    "sentinel",
    "set",
    "setex",
    "sismember",
    "smembers",
    "spublish",
    "srandmember",
    "srem",
    "sscan",
    "strlen",
    "subscribe",
    "time",
    "ttl",
    "type",
    "unlink",
    "unsubscribe",
    "zadd",
    "zcard",
    "zcount",
    "zrange",
    "zrangebyscore",
    "zrem",
    "zremrangebyrank",
    "zremrangebyscore",
    "zscan",
    "zscore",
};

struct ConnStateStatistic {
  std::array<size_t, static_cast<size_t>(Redis::State::kDisconnectError) + 1>
      statistic{};

  void Add(const ShardStatistics& shard_stats) {
    for (const auto& [_, stats] : shard_stats.instances) {
      UASSERT(stats.state <= Redis::State::kDisconnectError);
      statistic.at(static_cast<size_t>(stats.state))++;
    }
  }

  size_t Get(Redis::State state) const {
    UASSERT(state <= Redis::State::kDisconnectError);
    return statistic.at(static_cast<size_t>(state));
  }
};

void DumpMetric(utils::statistics::Writer& writer,
                const ConnStateStatistic& stats) {
  for (size_t i = 0; i <= static_cast<int>(Redis::State::kDisconnectError);
       ++i) {
    const auto state = static_cast<Redis::State>(i);
    writer["cluster_states"].ValueWithLabels(
        stats.Get(state),
        {"redis_instance_state", redis::Redis::StateToString(state)});
  }
}

}  // namespace

std::chrono::milliseconds MillisecondsSinceEpoch() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now().time_since_epoch());
}

Statistics::Statistics() {
  command_timings_percentile.reserve(std::size(kCommandTypes));
  for (const auto& cmd : kCommandTypes)
    command_timings_percentile.try_emplace(cmd);
}

void Statistics::AccountStateChanged(RedisState new_state) {
  if (state.load() != RedisState::kConnected &&
      new_state == RedisState::kConnected) {
    reconnects++;
    session_start_time = MillisecondsSinceEpoch();
  }
  state = new_state;
}

void Statistics::AccountCommandSent(const CommandPtr& cmd) {
  for (const auto& args : cmd->args.args) {
    size_t size = 0;
    for (const auto& arg : args) size += arg.size();
    request_size_percentile.GetCurrentCounter().Account(size);
  }
}

void Statistics::AccountReplyReceived(const ReplyPtr& reply,
                                      const CommandPtr& cmd) {
  reply_size_percentile.GetCurrentCounter().Account(reply->data.GetSize());
  auto start = cmd->GetStartHandlingTime();
  auto delta = std::chrono::steady_clock::now() - start;
  auto ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(delta).count();
  timings_percentile.GetCurrentCounter().Account(ms);
  auto command_timings = command_timings_percentile.find(cmd->GetName());
  if (command_timings != command_timings_percentile.end()) {
    command_timings->second.GetCurrentCounter().Account(ms);
  } else {
    LOG_LIMITED_WARNING() << "Cannot account timings for unknown command '"
                          << cmd->GetName() << '\'';
    UASSERT_MSG(false, "Cannot account timings for unknown command");
  }

  AccountError(reply->status);
}

void Statistics::AccountError(ReplyStatus code) {
  error_count[static_cast<int>(code)]++;
}

void Statistics::AccountPing(std::chrono::milliseconds ping) {
  last_ping_ms = ping.count();
}

InstanceStatistics SentinelStatistics::GetShardGroupTotalStatistics() const {
  return shard_group_total;
}

void DumpMetric(utils::statistics::Writer& writer,
                const InstanceStatistics& stats, bool real_instance) {
  writer["reconnects"] = stats.reconnects;

  if (stats.settings.IsRequestSizesEnabled()) {
    writer["request_sizes"] = stats.request_size_percentile;
  }
  if (stats.settings.IsReplySizesEnabled()) {
    writer["reply_sizes"] = stats.reply_size_percentile;
  }
  if (stats.settings.IsTimingsEnabled()) {
    writer["timings"] = stats.timings_percentile;
  }

  if (stats.settings.IsCommandTimingsEnabled() &&
      !stats.command_timings_percentile.empty()) {
    for (const auto& [command, percentile] : stats.command_timings_percentile) {
      writer["command_timings"].ValueWithLabels(percentile,
                                                {"redis_command", command});
    }
  }

  for (size_t i = 0; i < kReplyStatusMap.size(); ++i) {
    writer["errors"].ValueWithLabels(
        stats.error_count[i],
        {"redis_error", ToString(static_cast<ReplyStatus>(i))});
  }

  if (real_instance) {
    writer["last_ping_ms"] = stats.last_ping_ms;
    writer["is_syncing"] = static_cast<int>(stats.is_syncing);
    writer["offset_from_master"] = stats.offset_from_master;

    for (size_t i = 0; i <= static_cast<int>(Redis::State::kDisconnectError);
         ++i) {
      const auto state = static_cast<Redis::State>(i);
      writer["state"].ValueWithLabels(
          static_cast<int>(stats.state == state),
          {"redis_instance_state", redis::Redis::StateToString(state)});
    }

    long long session_time_ms =
        stats.state == redis::Redis::State::kConnected
            ? (redis::MillisecondsSinceEpoch() - stats.session_start_time)
                  .count()
            : 0;
    writer["session-time-ms"] = session_time_ms;
  }
}

void DumpMetric(utils::statistics::Writer& writer,
                const ShardStatistics& stats) {
  const auto& settings = stats.shard_total.settings;
  const auto not_ready =
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::steady_clock::now() - stats.last_ready_time)
          .count();
  writer["is_ready"] = stats.is_ready;
  writer["not_ready_ms"] = stats.is_ready ? 0 : not_ready;
  // writer["shard-total"] = stats.shard_total;
  writer["instances_count"] = stats.instances.size();
  DumpMetric(writer, stats.shard_total, false);
  if (settings.GetMetricsLevel() >= MetricsSettings::Level::kInstance) {
    for (const auto& [inst_name, inst_stats] : stats.instances) {
      writer.ValueWithLabels(inst_stats, {"redis_instance", inst_name});
    }
  }
}

void DumpMetric(utils::statistics::Writer& writer,
                const SentinelStatistics& stats) {
  const auto& settings = stats.shard_group_total.settings;
  DumpMetric(writer, stats.shard_group_total, false);
  writer["errors"].ValueWithLabels(stats.internal.redis_not_ready.load(),
                                   {"redis_error", "redis_not_ready"});
  if (stats.internal.is_autotoplogy.load()) {
    writer["cluster_topology_checks"] =
        stats.internal.cluster_topology_checks.load();
    writer["cluster_topology_updates"] =
        stats.internal.cluster_topology_updates.load();
  }

  ConnStateStatistic conn_stat_masters;
  for (const auto& [shard_name, shard_stats] : stats.masters) {
    if (settings.GetMetricsLevel() >= MetricsSettings::Level::kShard) {
      writer.ValueWithLabels(shard_stats, {{"redis_instance_type", "masters"},
                                           {"redis_shard", shard_name}});
    }
    conn_stat_masters.Add(shard_stats);
  }
  writer.ValueWithLabels(conn_stat_masters,
                         {{"redis_instance_type", "masters"}});

  ConnStateStatistic conn_stat_slaves;
  for (const auto& [shard_name, shard_stats] : stats.slaves) {
    if (settings.GetMetricsLevel() >= MetricsSettings::Level::kShard) {
      writer.ValueWithLabels(shard_stats, {{"redis_instance_type", "slaves"},
                                           {"redis_shard", shard_name}});
    }
    conn_stat_slaves.Add(shard_stats);
  }
  writer.ValueWithLabels(conn_stat_slaves, {{"redis_instance_type", "slaves"}});

  if (stats.sentinel) {
    writer.ValueWithLabels(*stats.sentinel,
                           {"redis_instance_type", "sentinels"});
    ConnStateStatistic conn_stat;
    conn_stat.Add(stats.sentinel.value());
    writer.ValueWithLabels(conn_stat, {{"redis_instance_type", "sentinels"}});
  }
}

}  // namespace redis

USERVER_NAMESPACE_END
