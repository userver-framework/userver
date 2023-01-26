#include <userver/storages/redis/impl/redis_stats.hpp>

#include <storages/redis/impl/command.hpp>
#include <userver/logging/log.hpp>
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

void Statistics::AccountError(int code) {
  if (code >= 0 && code < static_cast<int>(error_count.size())) {
    error_count[code]++;
  } else {
    error_count[REDIS_ERR_OTHER]++;
  }
}

void Statistics::AccountPing(std::chrono::milliseconds ping) {
  last_ping_ms = ping.count();
}

InstanceStatistics ShardStatistics::GetShardTotalStatistics() const {
  redis::InstanceStatistics shard_total;
  for (const auto& it2 : instances) {
    const auto& inst_stats = it2.second;
    shard_total.Add(inst_stats);
  }
  return shard_total;
}

InstanceStatistics SentinelStatistics::GetShardGroupTotalStatistics() const {
  redis::InstanceStatistics shard_group_total;
  for (const auto& it : masters) {
    shard_group_total.Add(it.second.GetShardTotalStatistics());
  }
  for (const auto& it : slaves) {
    shard_group_total.Add(it.second.GetShardTotalStatistics());
  }
  return shard_group_total;
}

}  // namespace redis

USERVER_NAMESPACE_END
