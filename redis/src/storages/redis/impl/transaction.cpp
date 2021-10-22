#include "transaction.hpp"

#include <sstream>

#include <userver/storages/redis/impl/reply.hpp>
#include <userver/storages/redis/impl/sentinel.hpp>
#include "redis.hpp"

USERVER_NAMESPACE_BEGIN

namespace redis {

Transaction& Transaction::Append(const std::string& key,
                                 const std::string& value) {
  return AddCmd(key, "append", key, value);
}

Transaction& Transaction::Bitcount(const std::string& key) {
  return AddCmd(key, "bitcount", key);
}

Transaction& Transaction::Bitcount(const std::string& key, int64_t start,
                                   int64_t end) {
  return AddCmd(key, "bitcount", key, start, end);
}

Transaction& Transaction::Bitop(const std::string& operation,
                                const std::string& destkey,
                                const std::string& key) {
  return AddCmd(key, "bitop", operation, destkey, key);
}

Transaction& Transaction::Bitop(const std::string& operation,
                                const std::string& destkey,
                                const std::vector<std::string>& keys) {
  return AddCmd(keys, "bitop", operation, destkey, keys);
}

Transaction& Transaction::Bitpos(const std::string& key, int bit) {
  return AddCmd(key, "bitpos", key, bit);
}

Transaction& Transaction::Bitpos(const std::string& key, int bit,
                                 int64_t start) {
  return AddCmd(key, "bitpos", key, bit, start);
}

Transaction& Transaction::Bitpos(const std::string& key, int bit, int64_t start,
                                 int64_t end) {
  return AddCmd(key, "bitpos", key, bit, start, end);
}

Transaction& Transaction::Blpop(const std::string& key, int64_t timeout) {
  return AddCmd(key, "blpop", key, timeout);
}

Transaction& Transaction::Blpop(const std::vector<std::string>& keys,
                                int64_t timeout) {
  return AddCmd(keys, "blpop", keys, timeout);
}

Transaction& Transaction::Brpop(const std::string& key, int64_t timeout) {
  return AddCmd(key, "brpop", key, timeout);
}

Transaction& Transaction::Brpop(const std::vector<std::string>& keys,
                                int64_t timeout) {
  return AddCmd(keys, "brpop", keys, timeout);
}

Transaction& Transaction::Decr(const std::string& key) {
  return AddCmd(key, "decr", key);
}

Transaction& Transaction::Decrby(const std::string& key, int64_t val) {
  return AddCmd(key, "decrby", key, val);
}

Transaction& Transaction::Del(const std::string& key) {
  return AddCmd(key, "del", key);
}

Transaction& Transaction::Del(const std::vector<std::string>& keys) {
  return AddCmd(keys, "del", keys);
}

Transaction& Transaction::Dump(const std::string& key) {
  return AddCmd(key, "dump", key);
}

Transaction& Transaction::Eval(const std::string& script,
                               const std::vector<std::string>& keys,
                               const std::vector<std::string>& args) {
  return AddCmd(keys, "eval", script, keys.size(), keys, args);
}

Transaction& Transaction::Evalsha(const std::string& sha1,
                                  const std::vector<std::string>& keys,
                                  const std::vector<std::string>& args) {
  return AddCmd(keys, "evalsha", sha1, keys.size(), keys, args);
}

Transaction& Transaction::Exists(const std::string& key) {
  return AddCmd(key, "exists", key);
}

Transaction& Transaction::Exists(const std::vector<std::string>& keys) {
  return AddCmd(keys, "exists", keys);
}

Transaction& Transaction::Expire(const std::string& key, int64_t seconds) {
  return AddCmd(key, "expire", key, seconds);
}

Transaction& Transaction::Expireat(const std::string& key, int64_t timestamp) {
  return AddCmd(key, "expireat", key, timestamp);
}

Transaction& Transaction::Geoadd(const std::string& key,
                                 const GeoaddArg& lon_lat_member) {
  return AddCmd(key, "geoadd", key, lon_lat_member);
}

Transaction& Transaction::Geoadd(const std::string& key,
                                 const std::vector<GeoaddArg>& lon_lat_member) {
  return AddCmd(key, "geoadd", key, lon_lat_member);
}

Transaction& Transaction::Geodist(const std::string& key,
                                  const std::string& member_1,
                                  const std::string& member_2) {
  return AddCmd(key, "geodist", key, member_1, member_2);
}

Transaction& Transaction::Geodist(const std::string& key,
                                  const std::string& member_1,
                                  const std::string& member_2,
                                  const std::string& unit) {
  return AddCmd(key, "geodist", key, member_1, member_2, unit);
}

Transaction& Transaction::Geohash(const std::string& key,
                                  const std::string& member) {
  return AddCmd(key, "geohash", key, member);
}

Transaction& Transaction::Geohash(const std::string& key,
                                  const std::vector<std::string>& members) {
  return AddCmd(key, "geohash", key, members);
}

Transaction& Transaction::Geopos(const std::string& key,
                                 const std::string& member) {
  return AddCmd(key, "geopos", key, member);
}

Transaction& Transaction::Geopos(const std::string& key,
                                 const std::vector<std::string>& members) {
  return AddCmd(key, "geopos", key, members);
}

Transaction& Transaction::Georadius(const std::string& key, double lon,
                                    double lat, double radius,
                                    const GeoradiusOptions& options) {
  return AddCmd(key, "georadius_ro", key, lon, lat, radius, options);
}

Transaction& Transaction::Georadius(const std::string& key, double lon,
                                    double lat, double radius,
                                    const std::string& unit,
                                    const GeoradiusOptions& options) {
  return AddCmd(key, "georadius_ro", key, lon, lat, radius, unit, options);
}

Transaction& Transaction::Georadiusbymember(const std::string& key,
                                            const std::string& member,
                                            double radius,
                                            const GeoradiusOptions& options) {
  return AddCmd(key, "georadiusbymember", key, member, radius, options);
}

Transaction& Transaction::Georadiusbymember(const std::string& key,
                                            const std::string& member,
                                            double radius,
                                            const std::string& unit,
                                            const GeoradiusOptions& options) {
  return AddCmd(key, "georadiusbymember", key, member, radius, unit, options);
}

Transaction& Transaction::Get(const std::string& key) {
  return AddCmd(key, "get", key);
}

Transaction& Transaction::Getbit(const std::string& key, int64_t offset) {
  return AddCmd(key, "getbit", key, offset);
}

Transaction& Transaction::Getrange(const std::string& key, int64_t start,
                                   int64_t end) {
  return AddCmd(key, "getrange", key, start, end);
}

Transaction& Transaction::Getset(const std::string& key,
                                 const std::string& val) {
  return AddCmd(key, "getset", key, val);
}

Transaction& Transaction::Hdel(const std::string& key,
                               const std::string& field) {
  return AddCmd(key, "hdel", key, field);
}

Transaction& Transaction::Hdel(const std::string& key,
                               const std::vector<std::string>& fields) {
  return AddCmd(key, "hdel", key, fields);
}

Transaction& Transaction::Hexists(const std::string& key,
                                  const std::string& field) {
  return AddCmd(key, "hexists", key, field);
}

Transaction& Transaction::Hget(const std::string& key,
                               const std::string& field) {
  return AddCmd(key, "hget", key, field);
}

Transaction& Transaction::Hgetall(const std::string& key) {
  return AddCmd(key, "hgetall", key);
}

Transaction& Transaction::Hincrby(const std::string& key,
                                  const std::string& field, int64_t incr) {
  return AddCmd(key, "hincrby", key, field, incr);
}

Transaction& Transaction::Hincrbyfloat(const std::string& key,
                                       const std::string& field, double incr) {
  return AddCmd(key, "hincrbyfloat", key, field, incr);
}

Transaction& Transaction::Hkeys(const std::string& key) {
  return AddCmd(key, "hkeys", key);
}

Transaction& Transaction::Hlen(const std::string& key) {
  return AddCmd(key, "hlen", key);
}

Transaction& Transaction::Hmget(const std::string& key,
                                const std::vector<std::string>& fields) {
  return AddCmd(key, "hmget", key, fields);
}

Transaction& Transaction::Hmset(
    const std::string& key,
    const std::vector<std::pair<std::string, std::string>>& field_val) {
  return AddCmd(key, "hmset", key, field_val);
}

Transaction& Transaction::Hset(const std::string& key, const std::string& field,
                               const std::string& value) {
  return AddCmd(key, "hset", key, field, value);
}

Transaction& Transaction::Hsetnx(const std::string& key,
                                 const std::string& field,
                                 const std::string& value) {
  return AddCmd(key, "hsetnx", key, field, value);
}

Transaction& Transaction::Hstrlen(const std::string& key,
                                  const std::string& field) {
  return AddCmd(key, "hstrlen", key, field);
}

Transaction& Transaction::Hvals(const std::string& key) {
  return AddCmd(key, "hvals", key);
}

Transaction& Transaction::Incr(const std::string& key) {
  return AddCmd(key, "incr", key);
}

Transaction& Transaction::Incrby(const std::string& key, int64_t incr) {
  return AddCmd(key, "incrby", key, incr);
}

Transaction& Transaction::Incrbyfloat(const std::string& key, double incr) {
  return AddCmd(key, "incrbyfloat", key, incr);
}

Transaction& Transaction::Lindex(const std::string& key, int64_t index) {
  return AddCmd(key, "lindex", key, index);
}

Transaction& Transaction::Linsert(const std::string& key,
                                  const std::string& before_after,
                                  const std::string& pivot,
                                  const std::string& value) {
  return AddCmd(key, "linsert", key, before_after, pivot, value);
}

Transaction& Transaction::Llen(const std::string& key) {
  return AddCmd(key, "llen", key);
}

Transaction& Transaction::Lpop(const std::string& key) {
  return AddCmd(key, "lpop", key);
}

Transaction& Transaction::Lpush(const std::string& key,
                                const std::string& value) {
  return AddCmd(key, "lpush", key, value);
}

Transaction& Transaction::Lpush(const std::string& key,
                                const std::vector<std::string>& values) {
  return AddCmd(key, "lpush", key, values);
}

Transaction& Transaction::Lpushx(const std::string& key,
                                 const std::string& value) {
  return AddCmd(key, "lpushx", key, value);
}

Transaction& Transaction::Lrange(const std::string& key, int64_t start,
                                 int64_t stop) {
  return AddCmd(key, "lrange", key, start, stop);
}

Transaction& Transaction::Lrem(const std::string& key, int64_t count,
                               const std::string& value) {
  return AddCmd(key, "lrem", key, count, value);
}

Transaction& Transaction::Lset(const std::string& key, int64_t index,
                               const std::string& value) {
  return AddCmd(key, "lset", key, index, value);
}

Transaction& Transaction::Ltrim(const std::string& key, int64_t start,
                                int64_t stop) {
  return AddCmd(key, "ltrim", key, start, stop);
}

Transaction& Transaction::Mget(const std::vector<std::string>& keys) {
  return AddCmd(keys, "mget", keys);
}

Transaction& Transaction::Persist(const std::string& key) {
  return AddCmd(key, "persist", key);
}

Transaction& Transaction::Pexpire(const std::string& key,
                                  int64_t milliseconds) {
  return AddCmd(key, "pexpire", key, milliseconds);
}

Transaction& Transaction::Pexpireat(const std::string& key,
                                    int64_t milliseconds_timestamp) {
  return AddCmd(key, "pexpireat", key, milliseconds_timestamp);
}

Transaction& Transaction::Pfadd(const std::string& key,
                                const std::string& element) {
  return AddCmd(key, "pfadd", key, element);
}

Transaction& Transaction::Pfadd(const std::string& key,
                                const std::vector<std::string>& elements) {
  return AddCmd(key, "pfadd", key, elements);
}

Transaction& Transaction::Pfcount(const std::string& key) {
  return AddCmd(key, "pfcount", key);
}

Transaction& Transaction::Pfcount(const std::vector<std::string>& keys) {
  return AddCmd(keys, "pfcount", keys);
}

Transaction& Transaction::Pfmerge(const std::string& key,
                                  const std::vector<std::string>& sourcekeys) {
  return AddCmd(key, "pfmerge", key, sourcekeys);
}

Transaction& Transaction::Psetex(const std::string& key, int64_t milliseconds,
                                 const std::string& val) {
  return AddCmd(key, "psetex", key, milliseconds, val);
}

Transaction& Transaction::Pttl(const std::string& key) {
  return AddCmd(key, "pttl", key);
}

Transaction& Transaction::Restore(const std::string& key, int64_t ttl,
                                  const std::string& serialized_value) {
  return AddCmd(key, "restore", key, ttl, serialized_value);
}

Transaction& Transaction::Restore(const std::string& key, int64_t ttl,
                                  const std::string& serialized_value,
                                  const std::string& replace) {
  return AddCmd(key, "restore", key, ttl, serialized_value, replace);
}

Transaction& Transaction::Rpop(const std::string& key) {
  return AddCmd(key, "rpop", key);
}

Transaction& Transaction::Rpoplpush(const std::string& key,
                                    const std::string& destination) {
  return AddCmd(key, "rpoplpush", key, destination);
}

Transaction& Transaction::Rpush(const std::string& key,
                                const std::string& value) {
  return AddCmd(key, "rpush", key, value);
}

Transaction& Transaction::Rpush(const std::string& key,
                                const std::vector<std::string>& values) {
  return AddCmd(key, "rpush", key, values);
}

Transaction& Transaction::Rpushx(const std::string& key,
                                 const std::string& value) {
  return AddCmd(key, "rpushx", key, value);
}

Transaction& Transaction::Sadd(const std::string& key,
                               const std::string& member) {
  return AddCmd(key, "sadd", key, member);
}

Transaction& Transaction::Sadd(const std::string& key,
                               const std::vector<std::string>& members) {
  return AddCmd(key, "sadd", key, members);
}

Transaction& Transaction::Scard(const std::string& key) {
  return AddCmd(key, "scard", key);
}

Transaction& Transaction::Sdiff(const std::vector<std::string>& keys) {
  return AddCmd(keys, "sdiff", keys);
}

Transaction& Transaction::Sdiffstore(const std::string& destination,
                                     const std::vector<std::string>& keys) {
  return AddCmd(keys, "sdiffstore", destination, keys);
}

Transaction& Transaction::Set(const std::string& key,
                              const std::string& value) {
  return AddCmd(key, "set", key, value);
}

Transaction& Transaction::Set(const std::string& key, const std::string& value,
                              const SetOptions& options) {
  return AddCmd(key, "set", key, value, options);
}

Transaction& Transaction::Setbit(const std::string& key, int64_t offset,
                                 const std::string& value) {
  return AddCmd(key, "setbit", key, offset, value);
}

Transaction& Transaction::Setex(const std::string& key, int64_t seconds,
                                const std::string& value) {
  return AddCmd(key, "setex", key, seconds, value);
}

Transaction& Transaction::Setnx(const std::string& key,
                                const std::string& value) {
  return AddCmd(key, "setnx", key, value);
}

Transaction& Transaction::Setrange(const std::string& key, int64_t offset,
                                   const std::string& value) {
  return AddCmd(key, "setrange", key, offset, value);
}

Transaction& Transaction::Sinter(const std::vector<std::string>& keys) {
  return AddCmd(keys, "sinter", keys);
}

Transaction& Transaction::Sinterstore(const std::string& destination,
                                      const std::vector<std::string>& keys) {
  return AddCmd(keys, "sinterstore", destination, keys);
}

Transaction& Transaction::Sismember(const std::string& key,
                                    const std::string& member) {
  return AddCmd(key, "sismember", key, member);
}

Transaction& Transaction::Smembers(const std::string& key) {
  return AddCmd(key, "smembers", key);
}

Transaction& Transaction::Smove(const std::string& key,
                                const std::string& destination,
                                const std::string& member) {
  return AddCmd(key, "smove", key, destination, member);
}

Transaction& Transaction::Spop(const std::string& key) {
  return AddCmd(key, "spop", key);
}

Transaction& Transaction::Spop(const std::string& key, int64_t count) {
  return AddCmd(key, "spop", key, count);
}

Transaction& Transaction::Srandmember(const std::string& key) {
  return AddCmd(key, "srandmember", key);
}

Transaction& Transaction::Srandmember(const std::string& key, int64_t count) {
  return AddCmd(key, "srandmember", key, count);
}

Transaction& Transaction::Srem(const std::string& key,
                               const std::string& members) {
  return AddCmd(key, "srem", key, members);
}

Transaction& Transaction::Srem(const std::string& key,
                               const std::vector<std::string>& members) {
  return AddCmd(key, "srem", key, members);
}

Transaction& Transaction::strlen(const std::string& key) {
  return AddCmd(key, "strlen", key);
}

Transaction& Transaction::Sunion(const std::vector<std::string>& keys) {
  return AddCmd(keys, "sunion", keys);
}

Transaction& Transaction::Sunionstore(const std::string& destination,
                                      const std::vector<std::string>& keys) {
  return AddCmd(keys, "sunionstore", destination, keys);
}

Transaction& Transaction::Ttl(const std::string& key) {
  return AddCmd(key, "ttl", key);
}

Transaction& Transaction::Type(const std::string& key) {
  return AddCmd(key, "type", key);
}

Transaction& Transaction::Zadd(const std::string& key, double score,
                               const std::string& member) {
  return AddCmd(key, "zadd", key, score, member);
}

Transaction& Transaction::Zcard(const std::string& key) {
  return AddCmd(key, "zcard", key);
}

Transaction& Transaction::Zcount(const std::string& key, double min,
                                 double max) {
  return AddCmd(key, "zcount", key, min, max);
}

Transaction& Transaction::Zcount(const std::string& key, const std::string& min,
                                 const std::string& max) {
  return AddCmd(key, "zcount", key, min, max);
}

Transaction& Transaction::Zincrby(const std::string& key, int64_t incr,
                                  const std::string& member) {
  return AddCmd(key, "zincrby", key, incr, member);
}

Transaction& Transaction::Zincrby(const std::string& key, double incr,
                                  const std::string& member) {
  return AddCmd(key, "zincrby", key, incr, member);
}

Transaction& Transaction::Zincrby(const std::string& key,
                                  const std::string& incr,
                                  const std::string& member) {
  return AddCmd(key, "zincrby", key, incr, member);
}

Transaction& Transaction::Zlexcount(const std::string& key,
                                    const std::string& min,
                                    const std::string& max) {
  return AddCmd(key, "zlexcount", key, min, max);
}

Transaction& Transaction::Zrange(const std::string& key, int64_t start,
                                 int64_t stop,
                                 const ScoreOptions& score_options) {
  return AddCmd(key, "zrange", key, start, stop, score_options);
}

Transaction& Transaction::Zrangebylex(const std::string& key,
                                      const std::string& min,
                                      const std::string& max,
                                      const RangeOptions& range_options) {
  return AddCmd(key, "zrangebylex", key, min, max, range_options);
}

Transaction& Transaction::Zrangebyscore(
    const std::string& key, double min, double max,
    const RangeScoreOptions& range_score_options) {
  return AddCmd(key, "zrangebyscore", key, min, max, range_score_options);
}

Transaction& Transaction::Zrangebyscore(
    const std::string& key, const std::string& min, const std::string& max,
    const RangeScoreOptions& range_score_options) {
  return AddCmd(key, "zrangebyscore", key, min, max, range_score_options);
}

Transaction& Transaction::Zrank(const std::string& key,
                                const std::string& member) {
  return AddCmd(key, "zrank", key, member);
}

Transaction& Transaction::Zrem(const std::string& key,
                               const std::string& member) {
  return AddCmd(key, "zrem", key, member);
}

Transaction& Transaction::Zrem(const std::string& key,
                               const std::vector<std::string>& members) {
  return AddCmd(key, "zrem", key, members);
}

Transaction& Transaction::Zremrangebylex(const std::string& key,
                                         const std::string& min,
                                         const std::string& max) {
  return AddCmd(key, "zremrangebylex", key, min, max);
}

Transaction& Transaction::Zremrangebyrank(const std::string& key, int64_t start,
                                          int64_t stop) {
  return AddCmd(key, "zremrangebyrank", key, start, stop);
}

Transaction& Transaction::Zremrangebyscore(const std::string& key, double min,
                                           double max) {
  return AddCmd(key, "zremrangebyscore", key, min, max);
}

Transaction& Transaction::Zremrangebyscore(const std::string& key,
                                           const std::string& min,
                                           const std::string& max) {
  return AddCmd(key, "zremrangebyscore", key, min, max);
}

Transaction& Transaction::Zrevrange(const std::string& key, int64_t start,
                                    int64_t stop,
                                    const ScoreOptions& score_options) {
  return AddCmd(key, "zrevrange", key, start, stop, score_options);
}

Transaction& Transaction::Zrevrangebylex(const std::string& key,
                                         const std::string& min,
                                         const std::string& max,
                                         const RangeOptions& range_options) {
  return AddCmd(key, "zrevrangebylex", key, min, max, range_options);
}

Transaction& Transaction::Zrevrangebyscore(
    const std::string& key, double min, double max,
    const RangeScoreOptions& range_score_options) {
  return AddCmd(key, "zrevrangebyscore", key, min, max, range_score_options);
}

Transaction& Transaction::Zrevrangebyscore(
    const std::string& key, const std::string& min, const std::string& max,
    const RangeScoreOptions& range_score_options) {
  return AddCmd(key, "zrevrangebyscore", key, min, max, range_score_options);
}

Transaction& Transaction::Zrevrank(const std::string& key,
                                   const std::string& member) {
  return AddCmd(key, "zrevrank", key, member);
}

Transaction& Transaction::Zscore(const std::string& key,
                                 const std::string& member) {
  return AddCmd(key, "zscore", key, member);
}

Request Transaction::Exec(const CommandControl& command_control) {
  if (!shard_) {
    throw std::runtime_error("shard is not set");
  }
  if (command_control.force_shard_idx)
    shard_ = *command_control.force_shard_idx;
  cmd_args_.Then("EXEC");
  return sentinel_->MakeRequest(std::move(cmd_args_), *shard_, true,
                                sentinel_->GetCommandControl(command_control),
                                true);
}

namespace {

void assert_same_shards(size_t shard, const Sentinel& sentinel,
                        const std::string& key) {
  const size_t shard_for_key = sentinel.ShardByKey(key);
  if (shard != shard_for_key) {
    std::ostringstream oss;
    oss << "redis::Transaction must deal with same shard across all "
        << "the operations. Shard " << shard << " was detected by "
        << "first command, but one of the commands used key '" << key
        << "' that belongs to shard " << shard_for_key;
    throw std::runtime_error(oss.str());
  }
}

void assert_same_shards(size_t shard, const Sentinel& sentinel,
                        std::vector<std::string>::const_iterator keys_begin,
                        std::vector<std::string>::const_iterator keys_end) {
  for (; keys_begin != keys_end; ++keys_begin)
    assert_same_shards(shard, sentinel, *keys_begin);
}

}  // anonymous namespace

template <class... Args>
Transaction& Transaction::AddCmd(const std::vector<std::string>& keys,
                                 const char* command, Args&&... args) {
  auto keys_begin = keys.cbegin();
  if (!shard_) {
    shard_ = sentinel_->ShardByKey(keys.at(0));
    ++keys_begin;
  }

  if (check_shards_ == CheckShards::kSame)
    assert_same_shards(*shard_, *sentinel_, keys_begin, keys.cend());

  cmd_args_.Then(command, std::forward<Args>(args)...);
  return *this;
}

template <class... Args>
Transaction& Transaction::AddCmd(const std::string& key, const char* command,
                                 Args&&... args) {
  if (!shard_)
    shard_ = sentinel_->ShardByKey(key);
  else if (check_shards_ == CheckShards::kSame)
    assert_same_shards(*shard_, *sentinel_, key);

  cmd_args_.Then(command, std::forward<Args>(args)...);
  return *this;
}

}  // namespace redis

USERVER_NAMESPACE_END
