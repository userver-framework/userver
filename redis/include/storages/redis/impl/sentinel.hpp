#pragma once

#include <string>
#include <unordered_map>

#include <boost/signals2.hpp>

#include <utils/swappingsmart.hpp>

#include <storages/redis/impl/base.hpp>
#include <storages/redis/impl/command.hpp>
#include <storages/redis/impl/command_options.hpp>
#include <storages/redis/impl/redis_stats.hpp>
#include <storages/redis/impl/request.hpp>
#include "keyshard.hpp"
#include "secdist_redis.hpp"
#include "thread_pools.hpp"

namespace engine::ev {
class ThreadControl;
}  // namespace engine::ev

namespace redis {

// We need only one thread for sentinels different from redis threads
const int kDefaultSentinelThreadPoolSize = 1;

// It works fine with 8 threads in driver_authorizer
const int kDefaultRedisThreadPoolSize = 8;

const auto kSentinelGetHostsCheckInterval = std::chrono::seconds(3);

// Forward declarations
class SentinelImpl;
class Shard;

class Sentinel {
 public:
  using ReadyChangeCallback = std::function<void(
      size_t shard, const std::string& shard_name, bool master, bool ready)>;

  Sentinel(const std::shared_ptr<ThreadPools>& thread_pools,
           const std::vector<std::string>& shards,
           const std::vector<ConnectionInfo>& conns,
           std::string shard_group_name, const std::string& client_name,
           const std::string& password, ReadyChangeCallback ready_callback,
           std::unique_ptr<KeyShard>&& key_shard = nullptr,
           CommandControl command_control = command_control_init,
           bool track_masters = true, bool track_slaves = true);
  virtual ~Sentinel();

  // Wait until connections to all servers are up
  void WaitConnectedDebug();

  void ForceUpdateHosts();

  static std::shared_ptr<redis::Sentinel> CreateSentinel(
      const std::shared_ptr<ThreadPools>& thread_pools,
      const secdist::RedisSettings& settings, std::string shard_group_name,
      const std::string& client_name, KeyShardFactory key_shard_factory);
  static std::shared_ptr<redis::Sentinel> CreateSentinel(
      const std::shared_ptr<ThreadPools>& thread_pools,
      const secdist::RedisSettings& settings, std::string shard_group_name,
      const std::string& client_name, ReadyChangeCallback ready_callback,
      KeyShardFactory key_shard_factory);

  void Restart();

  std::unordered_map<ServerId, size_t, ServerIdHasher>
  GetAvailableServersWeighted(size_t shard_idx, bool with_master,
                              const CommandControl& cc = {}) const;

  void AsyncCommand(CommandPtr command, bool master = true, size_t shard = 0);
  void AsyncCommand(CommandPtr command, const std::string& key,
                    bool master = true);

  // return a new temporary key with the same shard index
  static std::string CreateTmpKey(const std::string& key,
                                  std::string prefix = "tmp:");

  size_t ShardByKey(const std::string& key) const;
  size_t ShardsCount() const;
  void CheckShardIdx(size_t shard_idx) const;
  static void CheckShardIdx(size_t shard_idx, size_t shard_count);

  // Returns a non-empty key of the minimum length consisting of lowercase
  // letters for a given shard.
  const std::string& GetAnyKeyForShard(size_t shard_idx) const;

  SentinelStatistics GetStatistics() const;

  boost::signals2::signal<void(size_t shard, bool master)>
      signal_instances_changed;

  Request MakeRequest(CmdArgs&& args, const std::string& key,
                      bool master = true,
                      const CommandControl& command_control = {},
                      bool skip_status = false) {
    return Request(*this, std::forward<CmdArgs>(args), key, master,
                   command_control, skip_status);
  }

  Request MakeRequest(CmdArgs&& args, size_t shard, bool master = true,
                      const CommandControl& command_control = {},
                      bool skip_status = false) {
    return Request(*this, std::forward<CmdArgs>(args), shard, master,
                   command_control, skip_status);
  }

  Request MakeRequest(CmdArgs&& args, size_t shard,
                      redis::ReplyCallback&& callback, bool master = true,
                      const CommandControl& command_control = {}) {
    return Request(*this, std::forward<CmdArgs>(args), shard, master,
                   command_control, std::move(callback));
  }

  std::vector<Request> MakeRequests(CmdArgs&& args, bool master = true,
                                    const CommandControl& command_control = {},
                                    bool skip_status = false);

  Request Append(const std::string& key, const std::string& value,
                 const CommandControl& command_control = CommandControl());
  Request Bitcount(const std::string& key,
                   const CommandControl& command_control = CommandControl());
  Request Bitcount(const std::string& key, int64_t start, int64_t end,
                   const CommandControl& command_control = CommandControl());
  Request Bitop(const std::string& operation, const std::string& destkey,
                const std::string& key,
                const CommandControl& command_control = CommandControl());
  Request Bitop(const std::string& operation, const std::string& destkey,
                const std::vector<std::string>& keys,
                const CommandControl& command_control = CommandControl());
  Request Bitpos(const std::string& key, int bit,
                 const CommandControl& command_control = CommandControl());
  Request Bitpos(const std::string& key, int bit, int64_t start,
                 const CommandControl& command_control = CommandControl());
  Request Bitpos(const std::string& key, int bit, int64_t start, int64_t end,
                 const CommandControl& command_control = CommandControl());
  Request Blpop(const std::string& key, int64_t timeout,
                const CommandControl& command_control = CommandControl());
  Request Blpop(const std::vector<std::string>& keys, int64_t timeout,
                const CommandControl& command_control = CommandControl());
  Request Brpop(const std::string& key, int64_t timeout,
                const CommandControl& command_control = CommandControl());
  Request Brpop(const std::vector<std::string>& keys, int64_t timeout,
                const CommandControl& command_control = CommandControl());
  Request DbsizeEstimated(
      size_t shard, const CommandControl& command_control = CommandControl());
  Request Decr(const std::string& key,
               const CommandControl& command_control = CommandControl());
  Request Decrby(const std::string& key, int64_t val,
                 const CommandControl& command_control = CommandControl());
  Request Del(const std::string& key,
              const CommandControl& command_control = CommandControl());
  Request Del(const std::vector<std::string>& keys,
              const CommandControl& command_control = CommandControl());
  Request Dump(const std::string& key,
               const CommandControl& command_control = CommandControl());
  Request Eval(const std::string& script, const std::vector<std::string>& keys,
               const std::vector<std::string>& args,
               const CommandControl& command_control = CommandControl());
  Request Evalsha(const std::string& sha1, const std::vector<std::string>& keys,
                  const std::vector<std::string>& args,
                  const CommandControl& command_control = CommandControl());
  Request Exists(const std::string& key,
                 const CommandControl& command_control = CommandControl());
  Request Exists(const std::vector<std::string>& keys,
                 const CommandControl& command_control = CommandControl());
  Request Expire(const std::string& key, std::chrono::seconds seconds,
                 const CommandControl& command_control = CommandControl()) {
    return Expire(key, seconds.count(), command_control);
  }
  Request Expire(const std::string& key, int64_t seconds,
                 const CommandControl& command_control = CommandControl());
  Request Expireat(const std::string& key, int64_t timestamp,
                   const CommandControl& command_control = CommandControl());
  std::vector<Request> FlushAll(
      const CommandControl& command_control = CommandControl());
  Request Geoadd(const std::string& key, const GeoaddArg& lon_lat_member,
                 const CommandControl& command_control = CommandControl());
  Request Geoadd(const std::string& key,
                 const std::vector<GeoaddArg>& lon_lat_member,
                 const CommandControl& command_control = CommandControl());
  Request Geodist(const std::string& key, const std::string& member_1,
                  const std::string& member_2,
                  const CommandControl& command_control = CommandControl());
  Request Geodist(const std::string& key, const std::string& member_1,
                  const std::string& member_2, const std::string& unit,
                  const CommandControl& command_control = CommandControl());
  Request Geohash(const std::string& key, const std::string& member,
                  const CommandControl& command_control = CommandControl());
  Request Geohash(const std::string& key,
                  const std::vector<std::string>& members,
                  const CommandControl& command_control = CommandControl());
  Request Geopos(const std::string& key, const std::string& member,
                 const CommandControl& command_control = CommandControl());
  Request Geopos(const std::string& key,
                 const std::vector<std::string>& members,
                 const CommandControl& command_control = CommandControl());
  Request Georadius(const std::string& key, double lon, double lat,
                    double radius, const GeoradiusOptions& options,
                    const CommandControl& command_control = CommandControl());
  Request Georadius(const std::string& key, double lon, double lat,
                    double radius, const std::string& unit,
                    const GeoradiusOptions& options,
                    const CommandControl& command_control = CommandControl());
  Request Georadiusbymember(
      const std::string& key, const std::string& member, double radius,
      const GeoradiusOptions& options,
      const CommandControl& command_control = CommandControl());
  Request Georadiusbymember(
      const std::string& key, const std::string& member, double radius,
      const std::string& unit, const GeoradiusOptions& options,
      const CommandControl& command_control = CommandControl());
  Request Get(const std::string& key,
              const CommandControl& command_control = CommandControl());
  Request Get(const std::string& key, RetryNilFromMaster,
              const CommandControl& command_control = CommandControl());
  Request Getbit(const std::string& key, int64_t offset,
                 const CommandControl& command_control = CommandControl());
  Request Getrange(const std::string& key, int64_t start, int64_t end,
                   const CommandControl& command_control = CommandControl());
  Request Getset(const std::string& key, const std::string& val,
                 const CommandControl& command_control = CommandControl());
  Request Hdel(const std::string& key, const std::string& field,
               const CommandControl& command_control = CommandControl());
  Request Hdel(const std::string& key, const std::vector<std::string>& fields,
               const CommandControl& command_control = CommandControl());
  Request Hexists(const std::string& key, const std::string& field,
                  const CommandControl& command_control = CommandControl());
  Request Hget(const std::string& key, const std::string& field,
               const CommandControl& command_control = CommandControl());
  Request Hgetall(const std::string& key,
                  const CommandControl& command_control = CommandControl());
  Request Hincrby(const std::string& key, const std::string& field,
                  int64_t incr,
                  const CommandControl& command_control = CommandControl());
  Request Hincrbyfloat(
      const std::string& key, const std::string& field, double incr,
      const CommandControl& command_control = CommandControl());
  Request Hkeys(const std::string& key,
                const CommandControl& command_control = CommandControl());
  Request Hlen(const std::string& key,
               const CommandControl& command_control = CommandControl());
  Request Hmget(const std::string& key, const std::vector<std::string>& fields,
                const CommandControl& command_control = CommandControl());
  Request Hmset(
      const std::string& key,
      const std::vector<std::pair<std::string, std::string>>& field_val,
      const CommandControl& command_control = CommandControl());
  Request Hset(const std::string& key, const std::string& field,
               const std::string& value,
               const CommandControl& command_control = CommandControl());
  Request Hsetnx(const std::string& key, const std::string& field,
                 const std::string& value,
                 const CommandControl& command_control = CommandControl());
  Request Hstrlen(const std::string& key, const std::string& field,
                  const CommandControl& command_control = CommandControl());
  Request Hvals(const std::string& key,
                const CommandControl& command_control = CommandControl());
  Request Incr(const std::string& key,
               const CommandControl& command_control = CommandControl());
  Request Incrby(const std::string& key, int64_t incr,
                 const CommandControl& command_control = CommandControl());
  Request Incrbyfloat(const std::string& key, double incr,
                      const CommandControl& command_control = CommandControl());
  Request Keys(const std::string& keys_pattern, size_t shard,
               const CommandControl& command_control = CommandControl());
  Request Lindex(const std::string& key, int64_t index,
                 const CommandControl& command_control = CommandControl());
  Request Linsert(const std::string& key, const std::string& before_after,
                  const std::string& pivot, const std::string& value,
                  const CommandControl& command_control = CommandControl());
  Request Llen(const std::string& key,
               const CommandControl& command_control = CommandControl());
  Request Lpop(const std::string& key,
               const CommandControl& command_control = CommandControl());
  Request Lpush(const std::string& key, const std::string& value,
                const CommandControl& command_control = CommandControl());
  Request Lpush(const std::string& key, const std::vector<std::string>& values,
                const CommandControl& command_control = CommandControl());
  Request Lpushx(const std::string& key, const std::string& value,
                 const CommandControl& command_control = CommandControl());
  Request Lrange(const std::string& key, int64_t start, int64_t stop,
                 const CommandControl& command_control = CommandControl());
  Request Lrem(const std::string& key, int64_t count, const std::string& value,
               const CommandControl& command_control = CommandControl());
  Request Lset(const std::string& key, int64_t index, const std::string& value,
               const CommandControl& command_control = CommandControl());
  Request Ltrim(const std::string& key, int64_t start, int64_t stop,
                const CommandControl& command_control = CommandControl());
  Request Mget(const std::vector<std::string>& keys,
               const CommandControl& command_control = CommandControl());
  Request Persist(const std::string& key,
                  const CommandControl& command_control = CommandControl());
  Request Pexpire(const std::string& key, int64_t milliseconds,
                  const CommandControl& command_control = CommandControl());
  Request Pexpire(const std::string& key,
                  std::chrono::milliseconds milliseconds,
                  const CommandControl& command_control = CommandControl()) {
    return Pexpire(key, milliseconds.count(), command_control);
  }
  Request Pexpireat(const std::string& key, int64_t milliseconds_timestamp,
                    const CommandControl& command_control = CommandControl());
  Request Pfadd(const std::string& key, const std::string& element,
                const CommandControl& command_control = CommandControl());
  Request Pfadd(const std::string& key,
                const std::vector<std::string>& elements,
                const CommandControl& command_control = CommandControl());
  Request Pfcount(const std::string& key,
                  const CommandControl& command_control = CommandControl());
  Request Pfcount(const std::vector<std::string>& keys,
                  const CommandControl& command_control = CommandControl());
  Request Pfmerge(const std::string& key,
                  const std::vector<std::string>& sourcekeys,
                  const CommandControl& command_control = CommandControl());
  Request Ping(const std::string& key,
               const CommandControl& command_control = CommandControl());
  Request Psetex(const std::string& key, int64_t milliseconds,
                 const std::string& val,
                 const CommandControl& command_control = CommandControl());
  Request Pttl(const std::string& key,
               const CommandControl& command_control = CommandControl());
  Request Restore(const std::string& key, int64_t ttl,
                  const std::string& serialized_value,
                  const CommandControl& command_control = CommandControl());
  Request Restore(const std::string& key, int64_t ttl,
                  const std::string& serialized_value,
                  const std::string& replace,
                  const CommandControl& command_control = CommandControl());
  Request RenameWithinShard(
      const std::string& key, const std::string& newkey,
      const CommandControl& command_control = CommandControl());
  Request Rpop(const std::string& key,
               const CommandControl& command_control = CommandControl());
  Request Rpoplpush(const std::string& key, const std::string& destination,
                    const CommandControl& command_control = CommandControl());
  Request Rpush(const std::string& key, const std::string& value,
                const CommandControl& command_control = CommandControl());
  Request Rpush(const std::string& key, const std::string& value, size_t shard,
                const CommandControl& command_control = CommandControl());
  Request Rpush(const std::string& key, const std::vector<std::string>& values,
                const CommandControl& command_control = CommandControl());
  Request Rpushx(const std::string& key, const std::string& value,
                 const CommandControl& command_control = CommandControl());
  Request Sadd(const std::string& key, const std::string& member,
               const CommandControl& command_control = CommandControl());
  Request Sadd(const std::string& key, const std::vector<std::string>& members,
               const CommandControl& command_control = CommandControl());
  Request Scan(size_t shard, const ScanOptions& options,
               const CommandControl& command_control = CommandControl());
  Request Scard(const std::string& key,
                const CommandControl& command_control = CommandControl());
  Request Sdiff(const std::vector<std::string>& keys,
                const CommandControl& command_control = CommandControl());
  Request Sdiffstore(const std::string& destination,
                     const std::vector<std::string>& keys,
                     const CommandControl& command_control = CommandControl());
  Request Set(const std::string& key, const std::string& value,
              const CommandControl& command_control = CommandControl());
  Request Set(const std::string& key, std::string&& value,
              const CommandControl& command_control = CommandControl());
  Request Set(const std::string& key, const std::string& value,
              const SetOptions& options,
              const CommandControl& command_control = CommandControl());
  Request Setbit(const std::string& key, int64_t offset,
                 const std::string& value,
                 const CommandControl& command_control = CommandControl());
  Request Setex(const std::string& key, int64_t seconds,
                const std::string& value,
                const CommandControl& command_control = CommandControl());
  Request Setnx(const std::string& key, const std::string& value,
                const CommandControl& command_control = CommandControl());
  Request Setrange(const std::string& key, int64_t offset,
                   const std::string& value,
                   const CommandControl& command_control = CommandControl());
  Request Sinter(const std::vector<std::string>& keys,
                 const CommandControl& command_control = CommandControl());
  Request Sinterstore(const std::string& destination,
                      const std::vector<std::string>& keys,
                      const CommandControl& command_control = CommandControl());
  Request Sismember(const std::string& key, const std::string& member,
                    const CommandControl& command_control = CommandControl());
  Request Smembers(const std::string& key,
                   const CommandControl& command_control = CommandControl());
  Request Smove(const std::string& key, const std::string& destination,
                const std::string& member,
                const CommandControl& command_control = CommandControl());
  Request Spop(const std::string& key,
               const CommandControl& command_control = CommandControl());
  Request Spop(const std::string& key, int64_t count,
               const CommandControl& command_control = CommandControl());
  Request Srandmember(const std::string& key,
                      const CommandControl& command_control = CommandControl());
  Request Srandmember(const std::string& key, int64_t count,
                      const CommandControl& command_control = CommandControl());
  Request Srem(const std::string& key, const std::string& members,
               const CommandControl& command_control = CommandControl());
  Request Srem(const std::string& key, const std::vector<std::string>& members,
               const CommandControl& command_control = CommandControl());
  // TODO: rename to Strlen
  Request strlen(const std::string& key,
                 const CommandControl& command_control = CommandControl());
  Request Sunion(const std::vector<std::string>& keys,
                 const CommandControl& command_control = CommandControl());
  Request Sunionstore(const std::string& destination,
                      const std::vector<std::string>& keys,
                      const CommandControl& command_control = CommandControl());
  Request Ttl(const std::string& key,
              const CommandControl& command_control = CommandControl());
  Request Type(const std::string& key,
               const CommandControl& command_control = CommandControl());
  Request Zadd(const std::string& key, double score, const std::string& member,
               const CommandControl& command_control = CommandControl());
  Request Zadd(const std::string& key, double score, const std::string& member,
               const ZaddOptions& options,
               const CommandControl& command_control = CommandControl());
  Request Zadd(
      const std::string& key,
      const std::vector<std::pair<double, std::string>>& scores_members,
      const CommandControl& command_control = CommandControl());
  Request Zadd(
      const std::string& key,
      const std::vector<std::pair<double, std::string>>& scores_members,
      const ZaddOptions& options,
      const CommandControl& command_control = CommandControl());
  Request Zcard(const std::string& key,
                const CommandControl& command_control = CommandControl());
  Request Zcount(const std::string& key, double min, double max,
                 const CommandControl& command_control = CommandControl());
  Request Zcount(const std::string& key, const std::string& min,
                 const std::string& max,
                 const CommandControl& command_control = CommandControl());
  Request Zincrby(const std::string& key, int64_t incr,
                  const std::string& member,
                  const CommandControl& command_control = CommandControl());
  Request Zincrby(const std::string& key, double incr,
                  const std::string& member,
                  const CommandControl& command_control = CommandControl());
  Request Zincrby(const std::string& key, const std::string& incr,
                  const std::string& member,
                  const CommandControl& command_control = CommandControl());
  Request Zlexcount(const std::string& key, const std::string& min,
                    const std::string& max,
                    const CommandControl& command_control = CommandControl());
  Request Zrange(const std::string& key, int64_t start, int64_t stop,
                 const ScoreOptions& score_options,
                 const CommandControl& command_control = CommandControl());
  Request Zrangebylex(const std::string& key, const std::string& min,
                      const std::string& max, const RangeOptions& range_options,
                      const CommandControl& command_control = CommandControl());
  Request Zrangebyscore(
      const std::string& key, double min, double max,
      const RangeScoreOptions& range_score_options,
      const CommandControl& command_control = CommandControl());
  Request Zrangebyscore(
      const std::string& key, const std::string& min, const std::string& max,
      const RangeScoreOptions& range_score_options,
      const CommandControl& command_control = CommandControl());
  Request Zrangebyscore(
      const std::string& key, const std::string& min, const std::string& max,
      const ScoreOptions& score_options,
      const CommandControl& command_control = CommandControl());
  Request Zrank(const std::string& key, const std::string& member,
                const CommandControl& command_control = CommandControl());
  Request Zrem(const std::string& key, const std::string& member,
               const CommandControl& command_control = CommandControl());
  Request Zrem(const std::string& key, const std::vector<std::string>& members,
               const CommandControl& command_control = CommandControl());
  Request Zremrangebylex(
      const std::string& key, const std::string& min, const std::string& max,
      const CommandControl& command_control = CommandControl());
  Request Zremrangebyrank(
      const std::string& key, int64_t start, int64_t stop,
      const CommandControl& command_control = CommandControl());
  Request Zremrangebyscore(
      const std::string& key, double min, double max,
      const CommandControl& command_control = CommandControl());
  Request Zremrangebyscore(
      const std::string& key, const std::string& min, const std::string& max,
      const CommandControl& command_control = CommandControl());
  Request Zrevrange(const std::string& key, int64_t start, int64_t stop,
                    const ScoreOptions& score_options,
                    const CommandControl& command_control = CommandControl());
  Request Zrevrangebylex(
      const std::string& key, const std::string& min, const std::string& max,
      const RangeOptions& range_options,
      const CommandControl& command_control = CommandControl());
  Request Zrevrangebyscore(
      const std::string& key, double min, double max,
      const RangeScoreOptions& range_score_options,
      const CommandControl& command_control = CommandControl());
  Request Zrevrangebyscore(
      const std::string& key, const std::string& min, const std::string& max,
      const RangeScoreOptions& range_score_options,
      const CommandControl& command_control = CommandControl());
  Request Zrevrank(const std::string& key, const std::string& member,
                   const CommandControl& command_control = CommandControl());
  Request Zscore(const std::string& key, const std::string& member,
                 const CommandControl& command_control = CommandControl());

  Request Publish(const std::string& channel, std::string message,
                  const CommandControl& command_control = CommandControl(),
                  PubShard policy = PubShard::kZeroShard);

  CommandControl GetCommandControl(const CommandControl& cc) const;

  virtual void SetConfigDefaultCommandControl(
      const std::shared_ptr<CommandControl>& cc);

  using UserMessageCallback = std::function<void(const std::string& channel,
                                                 const std::string& message)>;
  using UserPmessageCallback =
      std::function<void(const std::string& pattern, const std::string& channel,
                         const std::string& message)>;

  using MessageCallback =
      std::function<void(ServerId server_id, const std::string& channel,
                         const std::string& message)>;
  using PmessageCallback = std::function<void(
      ServerId server_id, const std::string& pattern,
      const std::string& channel, const std::string& message)>;
  using SubscribeCallback =
      std::function<void(ServerId, const std::string& channel, size_t count)>;
  using UnsubscribeCallback =
      std::function<void(ServerId, const std::string& channel, size_t count)>;

 protected:
  virtual void OnConnectionReady(size_t /*shard*/,
                                 const std::string& /*shard_name*/,
                                 bool /*master*/) {}

  std::vector<std::shared_ptr<const Shard>> GetMasterShards() const;

  std::unique_ptr<SentinelImpl> impl_;

 public:
  static void OnSubscribeReply(const MessageCallback message_callback,
                               const SubscribeCallback subscribe_callback,
                               const UnsubscribeCallback unsubscribe_callback,
                               ReplyPtr reply);

  static void OnPsubscribeReply(const PmessageCallback pmessage_callback,
                                const SubscribeCallback subscribe_callback,
                                const UnsubscribeCallback unsubscribe_callback,
                                ReplyPtr reply);

 private:
  size_t GetPublishShard(PubShard policy);

  void CheckRenameParams(const std::string& key, const std::string& newkey);

  friend class Transaction;

  std::shared_ptr<ThreadPools> thread_pools_;
  std::unique_ptr<engine::ev::ThreadControl> sentinel_thread_control_;
  CommandControl secdist_default_command_control_;
  utils::SwappingSmart<CommandControl> config_default_command_control_;
  std::atomic_int publish_shard_{0};
};

}  // namespace redis
