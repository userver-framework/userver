#include <storages/redis/impl/redis_stats.hpp>

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
    "zrange",
    "zrangebyscore",
    "zrem",
    "zremrangebyrank",
    "zremrangebyscore",
    "zscan",
    "zscore",
};

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

  if (stats.settings.request_sizes_enabled) {
    writer["request_sizes"] = stats.request_size_percentile;
  }
  if (stats.settings.reply_sizes_enabled) {
    writer["reply_sizes"] = stats.reply_size_percentile;
  }
  if (stats.settings.timings_enabled) {
    writer["timings"] = stats.timings_percentile;
  }

  if (stats.settings.command_timings_enabled &&
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
  const auto not_ready =
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::steady_clock::now() - stats.last_ready_time)
          .count();
  writer["is_ready"] = stats.is_ready;
  writer["not_ready_ms"] = stats.is_ready ? 0 : not_ready;
  // writer["shard-total"] = stats.shard_total;
  writer["instances_count"] = stats.instances.size();
  DumpMetric(writer, stats.shard_total, false);
  for (const auto& [inst_name, inst_stats] : stats.instances) {
    writer.ValueWithLabels(inst_stats, {"redis_instance", inst_name});
  }
}

void DumpMetric(utils::statistics::Writer& writer,
                const SentinelStatistics& stats) {
  DumpMetric(writer, stats.shard_group_total, false);
  writer["errors"].ValueWithLabels(stats.internal.redis_not_ready.load(),
                                   {"redis_error", "redis_not_ready"});
  for (const auto& [shard_name, shard_stats] : stats.masters) {
    writer.ValueWithLabels(shard_stats, {{"redis_instance_type", "masters"},
                                         {"redis_shard", shard_name}});
  }
  for (const auto& [shard_name, shard_stats] : stats.slaves) {
    writer.ValueWithLabels(shard_stats, {{"redis_instance_type", "slaves"},
                                         {"redis_shard", shard_name}});
  }
  if (stats.sentinel) {
    writer.ValueWithLabels(*stats.sentinel,
                           {"redis_instance_type", "sentinels"});
  }
}

}  // namespace redis

USERVER_NAMESPACE_END
