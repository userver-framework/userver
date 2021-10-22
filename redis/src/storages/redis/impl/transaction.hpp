#pragma once

#include <optional>

#include <userver/storages/redis/impl/base.hpp>
#include <userver/storages/redis/impl/command_options.hpp>
#include <userver/storages/redis/impl/request.hpp>
#include <userver/storages/redis/impl/types.hpp>

USERVER_NAMESPACE_BEGIN

namespace redis {

// NOTE
// all operations would be executed on one redis shard calculated by first key

class Transaction {
 public:
  enum class CheckShards { kNo, kSame };

  Transaction(std::shared_ptr<Sentinel> sentinel,
              CheckShards check_shards = CheckShards::kSame)
      : sentinel_(sentinel),
        check_shards_(check_shards),
        cmd_args_({"MULTI"}) {}

  Transaction& Append(const std::string& key, const std::string& value);
  Transaction& Bitcount(const std::string& key);
  Transaction& Bitcount(const std::string& key, int64_t start, int64_t end);
  Transaction& Bitop(const std::string& operation, const std::string& destkey,
                     const std::string& key);
  Transaction& Bitop(const std::string& operation, const std::string& destkey,
                     const std::vector<std::string>& keys);
  Transaction& Bitpos(const std::string& key, int bit);
  Transaction& Bitpos(const std::string& key, int bit, int64_t start);
  Transaction& Bitpos(const std::string& key, int bit, int64_t start,
                      int64_t end);
  Transaction& Blpop(const std::string& key, int64_t timeout);
  Transaction& Blpop(const std::vector<std::string>& keys, int64_t timeout);
  Transaction& Brpop(const std::string& key, int64_t timeout);
  Transaction& Brpop(const std::vector<std::string>& keys, int64_t timeout);
  Transaction& Decr(const std::string& key);
  Transaction& Decrby(const std::string& key, int64_t val);
  Transaction& Del(const std::string& key);
  Transaction& Del(const std::vector<std::string>& keys);
  Transaction& Dump(const std::string& key);
  Transaction& Eval(const std::string& script,
                    const std::vector<std::string>& keys,
                    const std::vector<std::string>& args);
  Transaction& Evalsha(const std::string& sha1,
                       const std::vector<std::string>& keys,
                       const std::vector<std::string>& args);
  Transaction& Exists(const std::string& key);
  Transaction& Exists(const std::vector<std::string>& keys);
  Transaction& Expire(const std::string& key, int64_t seconds);
  Transaction& Expireat(const std::string& key, int64_t timestamp);
  Transaction& Geoadd(const std::string& key, const GeoaddArg& lon_lat_member);
  Transaction& Geoadd(const std::string& key,
                      const std::vector<GeoaddArg>& lon_lat_member);
  Transaction& Geodist(const std::string& key, const std::string& member_1,
                       const std::string& member_2);
  Transaction& Geodist(const std::string& key, const std::string& member_1,
                       const std::string& member_2, const std::string& unit);
  Transaction& Geohash(const std::string& key, const std::string& member);
  Transaction& Geohash(const std::string& key,
                       const std::vector<std::string>& members);
  Transaction& Geopos(const std::string& key, const std::string& member);
  Transaction& Geopos(const std::string& key,
                      const std::vector<std::string>& members);
  Transaction& Georadius(const std::string& key, double lon, double lat,
                         double radius, const GeoradiusOptions& options);
  Transaction& Georadius(const std::string& key, double lon, double lat,
                         double radius, const std::string& unit,
                         const GeoradiusOptions& options);
  Transaction& Georadiusbymember(const std::string& key,
                                 const std::string& member, double radius,
                                 const GeoradiusOptions& options);
  Transaction& Georadiusbymember(const std::string& key,
                                 const std::string& member, double radius,
                                 const std::string& unit,
                                 const GeoradiusOptions& options);
  Transaction& Get(const std::string& key);
  Transaction& Getbit(const std::string& key, int64_t offset);
  Transaction& Getrange(const std::string& key, int64_t start, int64_t end);
  Transaction& Getset(const std::string& key, const std::string& val);
  Transaction& Hdel(const std::string& key, const std::string& field);
  Transaction& Hdel(const std::string& key,
                    const std::vector<std::string>& fields);
  Transaction& Hexists(const std::string& key, const std::string& field);
  Transaction& Hget(const std::string& key, const std::string& field);
  Transaction& Hgetall(const std::string& key);
  Transaction& Hincrby(const std::string& key, const std::string& field,
                       int64_t incr);
  Transaction& Hincrbyfloat(const std::string& key, const std::string& field,
                            double incr);
  Transaction& Hkeys(const std::string& key);
  Transaction& Hlen(const std::string& key);
  Transaction& Hmget(const std::string& key,
                     const std::vector<std::string>& fields);
  Transaction& Hmset(
      const std::string& key,
      const std::vector<std::pair<std::string, std::string>>& field_val);
  Transaction& Hset(const std::string& key, const std::string& field,
                    const std::string& value);
  Transaction& Hsetnx(const std::string& key, const std::string& field,
                      const std::string& value);
  Transaction& Hstrlen(const std::string& key, const std::string& field);
  Transaction& Hvals(const std::string& key);
  Transaction& Incr(const std::string& key);
  Transaction& Incrby(const std::string& key, int64_t incr);
  Transaction& Incrbyfloat(const std::string& key, double incr);
  Transaction& Lindex(const std::string& key, int64_t index);
  Transaction& Linsert(const std::string& key, const std::string& before_after,
                       const std::string& pivot, const std::string& value);
  Transaction& Llen(const std::string& key);
  Transaction& Lpop(const std::string& key);
  Transaction& Lpush(const std::string& key, const std::string& value);
  Transaction& Lpush(const std::string& key,
                     const std::vector<std::string>& values);
  Transaction& Lpushx(const std::string& key, const std::string& value);
  Transaction& Lrange(const std::string& key, int64_t start, int64_t stop);
  Transaction& Lrem(const std::string& key, int64_t count,
                    const std::string& value);
  Transaction& Lset(const std::string& key, int64_t index,
                    const std::string& value);
  Transaction& Ltrim(const std::string& key, int64_t start, int64_t stop);
  Transaction& Mget(const std::vector<std::string>& keys);
  Transaction& Persist(const std::string& key);
  Transaction& Pexpire(const std::string& key, int64_t milliseconds);
  Transaction& Pexpireat(const std::string& key,
                         int64_t milliseconds_timestamp);
  Transaction& Pfadd(const std::string& key, const std::string& element);
  Transaction& Pfadd(const std::string& key,
                     const std::vector<std::string>& elements);
  Transaction& Pfcount(const std::string& key);
  Transaction& Pfcount(const std::vector<std::string>& keys);
  Transaction& Pfmerge(const std::string& key,
                       const std::vector<std::string>& sourcekeys);
  Transaction& Psetex(const std::string& key, int64_t milliseconds,
                      const std::string& val);
  Transaction& Pttl(const std::string& key);
  Transaction& Restore(const std::string& key, int64_t ttl,
                       const std::string& serialized_value);
  Transaction& Restore(const std::string& key, int64_t ttl,
                       const std::string& serialized_value,
                       const std::string& replace);
  Transaction& Rpop(const std::string& key);
  Transaction& Rpoplpush(const std::string& key,
                         const std::string& destination);
  Transaction& Rpush(const std::string& key, const std::string& value);
  Transaction& Rpush(const std::string& key,
                     const std::vector<std::string>& values);
  Transaction& Rpushx(const std::string& key, const std::string& value);
  Transaction& Sadd(const std::string& key, const std::string& member);
  Transaction& Sadd(const std::string& key,
                    const std::vector<std::string>& members);
  Transaction& Scard(const std::string& key);
  Transaction& Sdiff(const std::vector<std::string>& keys);
  Transaction& Sdiffstore(const std::string& destination,
                          const std::vector<std::string>& keys);
  Transaction& Set(const std::string& key, const std::string& value);
  Transaction& Set(const std::string& key, const std::string& value,
                   const SetOptions& options);
  Transaction& Setbit(const std::string& key, int64_t offset,
                      const std::string& value);
  Transaction& Setex(const std::string& key, int64_t seconds,
                     const std::string& value);
  Transaction& Setnx(const std::string& key, const std::string& value);
  Transaction& Setrange(const std::string& key, int64_t offset,
                        const std::string& value);
  Transaction& Sinter(const std::vector<std::string>& keys);
  Transaction& Sinterstore(const std::string& destination,
                           const std::vector<std::string>& keys);
  Transaction& Sismember(const std::string& key, const std::string& member);
  Transaction& Smembers(const std::string& key);
  Transaction& Smove(const std::string& key, const std::string& destination,
                     const std::string& member);
  Transaction& Spop(const std::string& key);
  Transaction& Spop(const std::string& key, int64_t count);
  Transaction& Srandmember(const std::string& key);
  Transaction& Srandmember(const std::string& key, int64_t count);
  Transaction& Srem(const std::string& key, const std::string& members);
  Transaction& Srem(const std::string& key,
                    const std::vector<std::string>& members);
  Transaction& strlen(const std::string& key);
  Transaction& Sunion(const std::vector<std::string>& keys);
  Transaction& Sunionstore(const std::string& destination,
                           const std::vector<std::string>& keys);
  Transaction& Ttl(const std::string& key);
  Transaction& Type(const std::string& key);
  Transaction& Zadd(const std::string& key, double score,
                    const std::string& member);
  Transaction& Zcard(const std::string& key);
  Transaction& Zcount(const std::string& key, double min, double max);
  Transaction& Zcount(const std::string& key, const std::string& min,
                      const std::string& max);
  Transaction& Zincrby(const std::string& key, int64_t incr,
                       const std::string& member);
  Transaction& Zincrby(const std::string& key, double incr,
                       const std::string& member);
  Transaction& Zincrby(const std::string& key, const std::string& incr,
                       const std::string& member);
  Transaction& Zlexcount(const std::string& key, const std::string& min,
                         const std::string& max);
  Transaction& Zrange(const std::string& key, int64_t start, int64_t stop,
                      const ScoreOptions& score_options);
  Transaction& Zrangebylex(const std::string& key, const std::string& min,
                           const std::string& max,
                           const RangeOptions& range_options);
  Transaction& Zrangebyscore(const std::string& key, double min, double max,
                             const RangeScoreOptions& range_score_options);
  Transaction& Zrangebyscore(const std::string& key, const std::string& min,
                             const std::string& max,
                             const RangeScoreOptions& range_score_options);
  Transaction& Zrank(const std::string& key, const std::string& member);
  Transaction& Zrem(const std::string& key, const std::string& member);
  Transaction& Zrem(const std::string& key,
                    const std::vector<std::string>& members);
  Transaction& Zremrangebylex(const std::string& key, const std::string& min,
                              const std::string& max);
  Transaction& Zremrangebyrank(const std::string& key, int64_t start,
                               int64_t stop);
  Transaction& Zremrangebyscore(const std::string& key, double min, double max);
  Transaction& Zremrangebyscore(const std::string& key, const std::string& min,
                                const std::string& max);
  Transaction& Zrevrange(const std::string& key, int64_t start, int64_t stop,
                         const ScoreOptions& score_options);
  Transaction& Zrevrangebylex(const std::string& key, const std::string& min,
                              const std::string& max,
                              const RangeOptions& range_options);
  Transaction& Zrevrangebyscore(const std::string& key, double min, double max,
                                const RangeScoreOptions& range_score_options);
  Transaction& Zrevrangebyscore(const std::string& key, const std::string& min,
                                const std::string& max,
                                const RangeScoreOptions& range_score_options);
  Transaction& Zrevrank(const std::string& key, const std::string& member);
  Transaction& Zscore(const std::string& key, const std::string& member);

  Request Exec(const CommandControl& command_control = CommandControl());

 private:
  template <class... Args>
  Transaction& AddCmd(const std::string& key, const char* command,
                      Args&&... args);
  template <class... Args>
  Transaction& AddCmd(const std::vector<std::string>& keys, const char* command,
                      Args&&... args);

  std::shared_ptr<Sentinel> sentinel_;
  const CheckShards check_shards_;
  CmdArgs cmd_args_;
  std::optional<size_t> shard_;
};

}  // namespace redis

USERVER_NAMESPACE_END
