#include "transaction.hpp"

#include <engine/async.hpp>

namespace storages {
namespace redis {

Transaction& Transaction::Append(const std::string& key,
                                 const std::string& value) {
  cmd_args_.Then({"append", key, value});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Bitcount(const std::string& key) {
  cmd_args_.Then({"bitcount", key});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Bitcount(const std::string& key, int64_t start,
                                   int64_t end) {
  cmd_args_.Then({"bitcount", key, start, end});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Bitop(const std::string& operation,
                                const std::string& destkey,
                                const std::string& key) {
  cmd_args_.Then({"bitop", operation, destkey, key});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Bitop(const std::string& operation,
                                const std::string& destkey,
                                const std::vector<std::string>& keys) {
  cmd_args_.Then({"bitop", operation, destkey, keys});
  if (first_key_.empty()) first_key_ = keys[0];
  return *this;
}

Transaction& Transaction::Bitpos(const std::string& key, int bit) {
  cmd_args_.Then({"bitpos", key, bit});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Bitpos(const std::string& key, int bit,
                                 int64_t start) {
  cmd_args_.Then({"bitpos", key, bit, start});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Bitpos(const std::string& key, int bit, int64_t start,
                                 int64_t end) {
  cmd_args_.Then({"bitpos", key, bit, start, end});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Blpop(const std::string& key, int64_t timeout) {
  cmd_args_.Then({"blpop", key, timeout});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Blpop(const std::vector<std::string>& keys,
                                int64_t timeout) {
  cmd_args_.Then({"blpop", keys, timeout});
  if (first_key_.empty()) first_key_ = keys[0];
  return *this;
}

Transaction& Transaction::Brpop(const std::string& key, int64_t timeout) {
  cmd_args_.Then({"brpop", key, timeout});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Brpop(const std::vector<std::string>& keys,
                                int64_t timeout) {
  cmd_args_.Then({"brpop", keys, timeout});
  if (first_key_.empty()) first_key_ = keys[0];
  return *this;
}

Transaction& Transaction::ClusterKeyslot(const std::string& key) {
  cmd_args_.Then({"cluster_keyslot", key});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Decr(const std::string& key) {
  cmd_args_.Then({"decr", key});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Decrby(const std::string& key, int64_t val) {
  cmd_args_.Then({"decrby", key, val});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Del(const std::string& key) {
  cmd_args_.Then({"del", key});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Del(const std::vector<std::string>& keys) {
  cmd_args_.Then({"del", keys});
  if (first_key_.empty()) first_key_ = keys[0];
  return *this;
}

Transaction& Transaction::Dump(const std::string& key) {
  cmd_args_.Then({"dump", key});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Eval(const std::string& script, int64_t numkeys,
                               const std::vector<std::string>& keys,
                               const std::vector<std::string>& args) {
  cmd_args_.Then({"eval", script, numkeys, keys, args});
  if (first_key_.empty()) first_key_ = keys[0];
  return *this;
}

Transaction& Transaction::Evalsha(const std::string& sha1, int64_t numkeys,
                                  const std::vector<std::string>& keys,
                                  const std::vector<std::string>& args) {
  cmd_args_.Then({"evalsha", sha1, numkeys, keys, args});
  if (first_key_.empty()) first_key_ = keys[0];
  return *this;
}

Transaction& Transaction::Exists(const std::string& key) {
  cmd_args_.Then({"exists", key});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Exists(const std::vector<std::string>& keys) {
  cmd_args_.Then({"exists", keys});
  if (first_key_.empty()) first_key_ = keys[0];
  return *this;
}

Transaction& Transaction::Expire(const std::string& key,
                                 std::chrono::seconds seconds) {
  cmd_args_.Then({"expire", key, seconds.count()});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Expireat(const std::string& key, int64_t timestamp) {
  cmd_args_.Then({"expireat", key, timestamp});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Geoadd(const std::string& key,
                                 const GeoaddArg& lon_lat_member) {
  cmd_args_.Then({"geoadd", key, lon_lat_member});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Geoadd(const std::string& key,
                                 const std::vector<GeoaddArg>& lon_lat_member) {
  cmd_args_.Then({"geoadd", key, lon_lat_member});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Geodist(const std::string& key,
                                  const std::string& member_1,
                                  const std::string& member_2) {
  cmd_args_.Then({"geodist", key, member_1, member_2});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Geodist(const std::string& key,
                                  const std::string& member_1,
                                  const std::string& member_2,
                                  const std::string& unit) {
  cmd_args_.Then({"geodist", key, member_1, member_2, unit});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Geohash(const std::string& key,
                                  const std::string& member) {
  cmd_args_.Then({"geohash", key, member});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Geohash(const std::string& key,
                                  const std::vector<std::string>& members) {
  cmd_args_.Then({"geohash", key, members});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Geopos(const std::string& key,
                                 const std::string& member) {
  cmd_args_.Then({"geopos", key, member});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Geopos(const std::string& key,
                                 const std::vector<std::string>& members) {
  cmd_args_.Then({"geopos", key, members});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Georadius(const std::string& key, double lon,
                                    double lat, double radius,
                                    const GeoradiusOptions& options) {
  cmd_args_.Then({"georadius", key, lon, lat, radius, options});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Georadius(const std::string& key, double lon,
                                    double lat, double radius,
                                    const std::string& unit,
                                    const GeoradiusOptions& options) {
  cmd_args_.Then({"georadius", key, lon, lat, radius, unit, options});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Georadiusbymember(const std::string& key,
                                            const std::string& member,
                                            double radius,
                                            const GeoradiusOptions& options) {
  cmd_args_.Then({"georadiusbymember", key, member, radius, options});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Georadiusbymember(const std::string& key,
                                            const std::string& member,
                                            double radius,
                                            const std::string& unit,
                                            const GeoradiusOptions& options) {
  cmd_args_.Then({"georadiusbymember", key, member, radius, unit, options});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Get(const std::string& key) {
  cmd_args_.Then({"get", key});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Getbit(const std::string& key, int64_t offset) {
  cmd_args_.Then({"getbit", key, offset});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Getrange(const std::string& key, int64_t start,
                                   int64_t end) {
  cmd_args_.Then({"getrange", key, start, end});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Getset(const std::string& key,
                                 const std::string& val) {
  cmd_args_.Then({"getset", key, val});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Hdel(const std::string& key,
                               const std::string& field) {
  cmd_args_.Then({"hdel", key, field});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Hdel(const std::string& key,
                               const std::vector<std::string>& fields) {
  cmd_args_.Then({"hdel", key, fields});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Hexists(const std::string& key,
                                  const std::string& field) {
  cmd_args_.Then({"hexists", key, field});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Hget(const std::string& key,
                               const std::string& field) {
  cmd_args_.Then({"hget", key, field});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Hgetall(const std::string& key) {
  cmd_args_.Then({"hgetall", key});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Hincrby(const std::string& key,
                                  const std::string& field, int64_t incr) {
  cmd_args_.Then({"hincrby", key, field, incr});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Hincrbyfloat(const std::string& key,
                                       const std::string& field, double incr) {
  cmd_args_.Then({"hincrbyfloat", key, field, incr});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Hkeys(const std::string& key) {
  cmd_args_.Then({"hkeys", key});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Hlen(const std::string& key) {
  cmd_args_.Then({"hlen", key});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Hmget(const std::string& key,
                                const std::vector<std::string>& fields) {
  cmd_args_.Then({"hmget", key, fields});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Hmset(
    const std::string& key,
    const std::vector<std::pair<std::string, std::string>>& field_val) {
  cmd_args_.Then({"hmset", key, field_val});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Hset(const std::string& key, const std::string& field,
                               const std::string& value) {
  cmd_args_.Then({"hset", key, field, value});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Hsetnx(const std::string& key,
                                 const std::string& field,
                                 const std::string& value) {
  cmd_args_.Then({"hsetnx", key, field, value});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Hstrlen(const std::string& key,
                                  const std::string& field) {
  cmd_args_.Then({"hstrlen", key, field});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Hvals(const std::string& key) {
  cmd_args_.Then({"hvals", key});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Incr(const std::string& key) {
  cmd_args_.Then({"incr", key});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Incrby(const std::string& key, int64_t incr) {
  cmd_args_.Then({"incrby", key, incr});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Incrbyfloat(const std::string& key, double incr) {
  cmd_args_.Then({"incrbyfloat", key, incr});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Keys(const std::string& key) {
  cmd_args_.Then({"keys", key});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Lindex(const std::string& key, int64_t index) {
  cmd_args_.Then({"lindex", key, index});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Linsert(const std::string& key,
                                  const std::string& before_after,
                                  const std::string& pivot,
                                  const std::string& value) {
  cmd_args_.Then({"linsert", key, before_after, pivot, value});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Llen(const std::string& key) {
  cmd_args_.Then({"llen", key});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Lpop(const std::string& key) {
  cmd_args_.Then({"lpop", key});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Lpush(const std::string& key,
                                const std::string& value) {
  cmd_args_.Then({"lpush", key, value});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Lpush(const std::string& key,
                                const std::vector<std::string>& values) {
  cmd_args_.Then({"lpush", key, values});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Lpushx(const std::string& key,
                                 const std::string& value) {
  cmd_args_.Then({"lpushx", key, value});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Lrange(const std::string& key, int64_t start,
                                 int64_t stop) {
  cmd_args_.Then({"lrange", key, start, stop});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Lrem(const std::string& key, int64_t count,
                               const std::string& value) {
  cmd_args_.Then({"lrem", key, count, value});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Lset(const std::string& key, int64_t index,
                               const std::string& value) {
  cmd_args_.Then({"lset", key, index, value});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Ltrim(const std::string& key, int64_t start,
                                int64_t stop) {
  cmd_args_.Then({"ltrim", key, start, stop});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Mget(const std::vector<std::string>& keys) {
  cmd_args_.Then({"mget", keys});
  if (first_key_.empty()) first_key_ = keys[0];
  return *this;
}

Transaction& Transaction::Persist(const std::string& key) {
  cmd_args_.Then({"persist", key});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Pexpire(const std::string& key,
                                  std::chrono::milliseconds milliseconds) {
  cmd_args_.Then({"pexpire", key, milliseconds.count()});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Pexpireat(const std::string& key,
                                    int64_t milliseconds_timestamp) {
  cmd_args_.Then({"pexpireat", key, milliseconds_timestamp});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Pfadd(const std::string& key,
                                const std::string& element) {
  cmd_args_.Then({"pfadd", key, element});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Pfadd(const std::string& key,
                                const std::vector<std::string>& elements) {
  cmd_args_.Then({"pfadd", key, elements});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Pfcount(const std::string& key) {
  cmd_args_.Then({"pfcount", key});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Pfcount(const std::vector<std::string>& keys) {
  cmd_args_.Then({"pfcount", keys});
  if (first_key_.empty()) first_key_ = keys[0];
  return *this;
}

Transaction& Transaction::Pfmerge(const std::string& key,
                                  const std::vector<std::string>& sourcekeys) {
  cmd_args_.Then({"pfmerge", key, sourcekeys});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Psetex(const std::string& key,
                                 std::chrono::milliseconds milliseconds,
                                 const std::string& val) {
  cmd_args_.Then({"psetex", key, milliseconds.count(), val});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Pttl(const std::string& key) {
  cmd_args_.Then({"pttl", key});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Randomkey(size_t shard) {
  cmd_args_.Then({"randomkey"});
  if (first_key_.empty() && first_shard_ == static_cast<size_t>(-1))
    first_shard_ = shard;
  return *this;
}

Transaction& Transaction::Restore(const std::string& key, int64_t ttl,
                                  const std::string& serialized_value) {
  cmd_args_.Then({"restore", key, ttl, serialized_value});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Restore(const std::string& key, int64_t ttl,
                                  const std::string& serialized_value,
                                  const std::string& replace) {
  cmd_args_.Then({"restore", key, ttl, serialized_value, replace});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Rpop(const std::string& key) {
  cmd_args_.Then({"rpop", key});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Rpoplpush(const std::string& key,
                                    const std::string& destination) {
  cmd_args_.Then({"rpoplpush", key, destination});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Rpush(const std::string& key,
                                const std::string& value) {
  cmd_args_.Then({"rpush", key, value});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Rpush(const std::string& key,
                                const std::vector<std::string>& values) {
  cmd_args_.Then({"rpush", key, values});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Rpushx(const std::string& key,
                                 const std::string& value) {
  cmd_args_.Then({"rpushx", key, value});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Sadd(const std::string& key,
                               const std::string& member) {
  cmd_args_.Then({"sadd", key, member});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Sadd(const std::string& key,
                               const std::vector<std::string>& members) {
  cmd_args_.Then({"sadd", key, members});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Scard(const std::string& key) {
  cmd_args_.Then({"scard", key});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Sdiff(const std::vector<std::string>& keys) {
  cmd_args_.Then({"sdiff", keys});
  if (first_key_.empty()) first_key_ = keys[0];
  return *this;
}

Transaction& Transaction::Sdiffstore(const std::string& destination,
                                     const std::vector<std::string>& keys) {
  cmd_args_.Then({"sdiffstore", destination, keys});
  if (first_key_.empty()) first_key_ = keys[0];
  return *this;
}

Transaction& Transaction::Set(const std::string& key,
                              const std::string& value) {
  cmd_args_.Then({"set", key, value});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Set(const std::string& key, const std::string& value,
                              const SetOptions& options) {
  cmd_args_.Then({"set", key, value, options});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Setbit(const std::string& key, int64_t offset,
                                 const std::string& value) {
  cmd_args_.Then({"setbit", key, offset, value});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Setex(const std::string& key,
                                std::chrono::seconds seconds,
                                const std::string& value) {
  cmd_args_.Then({"setex", key, seconds.count(), value});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Setnx(const std::string& key,
                                const std::string& value) {
  cmd_args_.Then({"setnx", key, value});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Setrange(const std::string& key, int64_t offset,
                                   const std::string& value) {
  cmd_args_.Then({"setrange", key, offset, value});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Sinter(const std::vector<std::string>& keys) {
  cmd_args_.Then({"sinter", keys});
  if (first_key_.empty()) first_key_ = keys[0];
  return *this;
}

Transaction& Transaction::Sinterstore(const std::string& destination,
                                      const std::vector<std::string>& keys) {
  cmd_args_.Then({"sinterstore", destination, keys});
  if (first_key_.empty()) first_key_ = keys[0];
  return *this;
}

Transaction& Transaction::Sismember(const std::string& key,
                                    const std::string& member) {
  cmd_args_.Then({"sismember", key, member});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Smembers(const std::string& key) {
  cmd_args_.Then({"smembers", key});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Smove(const std::string& key,
                                const std::string& destination,
                                const std::string& member) {
  cmd_args_.Then({"smove", key, destination, member});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Spop(const std::string& key) {
  cmd_args_.Then({"spop", key});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Spop(const std::string& key, int64_t count) {
  cmd_args_.Then({"spop", key, count});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Srandmember(const std::string& key) {
  cmd_args_.Then({"srandmember", key});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Srandmember(const std::string& key, int64_t count) {
  cmd_args_.Then({"srandmember", key, count});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Srem(const std::string& key,
                               const std::string& members) {
  cmd_args_.Then({"srem", key, members});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Srem(const std::string& key,
                               const std::vector<std::string>& members) {
  cmd_args_.Then({"srem", key, members});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Strlen(const std::string& key) {
  cmd_args_.Then({"strlen", key});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Sunion(const std::vector<std::string>& keys) {
  cmd_args_.Then({"sunion", keys});
  if (first_key_.empty()) first_key_ = keys[0];
  return *this;
}

Transaction& Transaction::Sunionstore(const std::string& destination,
                                      const std::vector<std::string>& keys) {
  cmd_args_.Then({"sunionstore", destination, keys});
  if (first_key_.empty()) first_key_ = keys[0];
  return *this;
}

Transaction& Transaction::Ttl(const std::string& key) {
  cmd_args_.Then({"ttl", key});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Type(const std::string& key) {
  cmd_args_.Then({"type", key});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Zadd(const std::string& key, double score,
                               const std::string& member) {
  cmd_args_.Then({"zadd", key, score, member});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Zcard(const std::string& key) {
  cmd_args_.Then({"zcard", key});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Zcount(const std::string& key, int64_t min,
                                 int64_t max) {
  cmd_args_.Then({"zcount", key, min, max});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Zcount(const std::string& key, double min,
                                 double max) {
  cmd_args_.Then({"zcount", key, min, max});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Zcount(const std::string& key, const std::string& min,
                                 const std::string& max) {
  cmd_args_.Then({"zcount", key, min, max});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Zincrby(const std::string& key, int64_t incr,
                                  const std::string& member) {
  cmd_args_.Then({"zincrby", key, incr, member});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Zincrby(const std::string& key, double incr,
                                  const std::string& member) {
  cmd_args_.Then({"zincrby", key, incr, member});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Zincrby(const std::string& key,
                                  const std::string& incr,
                                  const std::string& member) {
  cmd_args_.Then({"zincrby", key, incr, member});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Zlexcount(const std::string& key, int64_t min,
                                    int64_t max) {
  cmd_args_.Then({"zlexcount", key, min, max});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Zlexcount(const std::string& key, double min,
                                    double max) {
  cmd_args_.Then({"zlexcount", key, min, max});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Zlexcount(const std::string& key,
                                    const std::string& min,
                                    const std::string& max) {
  cmd_args_.Then({"zlexcount", key, min, max});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Zrange(const std::string& key, int64_t start,
                                 int64_t stop,
                                 const ScoreOptions& score_options) {
  cmd_args_.Then({"zrange", key, start, stop, score_options});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Zrange(const std::string& key, double start,
                                 double stop,
                                 const ScoreOptions& score_options) {
  cmd_args_.Then({"zrange", key, start, stop, score_options});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Zrange(const std::string& key,
                                 const std::string& start,
                                 const std::string& stop,
                                 const ScoreOptions& score_options) {
  cmd_args_.Then({"zrange", key, start, stop, score_options});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Zrank(const std::string& key,
                                const std::string& member) {
  cmd_args_.Then({"zrank", key, member});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Zrem(const std::string& key,
                               const std::string& member) {
  cmd_args_.Then({"zrem", key, member});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Zrem(const std::string& key,
                               const std::vector<std::string>& members) {
  cmd_args_.Then({"zrem", key, members});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Zremrangebylex(const std::string& key, int64_t min,
                                         int64_t max) {
  cmd_args_.Then({"zremrangebylex", key, min, max});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Zremrangebylex(const std::string& key, double min,
                                         double max) {
  cmd_args_.Then({"zremrangebylex", key, min, max});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Zremrangebylex(const std::string& key,
                                         const std::string& min,
                                         const std::string& max) {
  cmd_args_.Then({"zremrangebylex", key, min, max});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Zremrangebyrank(const std::string& key, int64_t start,
                                          int64_t stop) {
  cmd_args_.Then({"zremrangebyrank", key, start, stop});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Zremrangebyrank(const std::string& key, double start,
                                          double stop) {
  cmd_args_.Then({"zremrangebyrank", key, start, stop});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Zremrangebyrank(const std::string& key,
                                          const std::string& start,
                                          const std::string& stop) {
  cmd_args_.Then({"zremrangebyrank", key, start, stop});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Zremrangebyscore(const std::string& key, int64_t min,
                                           int64_t max) {
  cmd_args_.Then({"zremrangebyscore", key, min, max});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Zremrangebyscore(const std::string& key, double min,
                                           double max) {
  cmd_args_.Then({"zremrangebyscore", key, min, max});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Zremrangebyscore(const std::string& key,
                                           const std::string& min,
                                           const std::string& max) {
  cmd_args_.Then({"zremrangebyscore", key, min, max});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Zrevrange(const std::string& key, int64_t start,
                                    int64_t stop,
                                    const ScoreOptions& score_options) {
  cmd_args_.Then({"zrevrange", key, start, stop, score_options});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Zrevrange(const std::string& key, double start,
                                    double stop,
                                    const ScoreOptions& score_options) {
  cmd_args_.Then({"zrevrange", key, start, stop, score_options});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Zrevrange(const std::string& key,
                                    const std::string& start,
                                    const std::string& stop,
                                    const ScoreOptions& score_options) {
  cmd_args_.Then({"zrevrange", key, start, stop, score_options});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Zrevrank(const std::string& key,
                                   const std::string& member) {
  cmd_args_.Then({"zrevrank", key, member});
  if (first_key_.empty()) first_key_ = key;
  return *this;
}

Transaction& Transaction::Zscore(const std::string& key,
                                 const std::string& member) {
  cmd_args_.Then({"zscore", key, member});
  if (first_key_.empty() && first_shard_ == static_cast<size_t>(-1))
    first_key_ = key;
  return *this;
}

Request Transaction::Exec(const CommandControl& command_control) {
  cmd_args_.Then({"EXEC"});
  if (first_shard_ == static_cast<size_t>(-1))
    return sentinel_->MakeRequest(std::move(cmd_args_), first_key_, true,
                                  sentinel_->GetCommandControl(command_control),
                                  true);
  else
    return sentinel_->MakeRequest(std::move(cmd_args_), first_shard_, true,
                                  sentinel_->GetCommandControl(command_control),
                                  true);
}

void Transaction::Exec(Callback&& callback,
                       const CommandControl& command_control) {
  cmd_args_.Then({"EXEC"});
  auto command = PrepareCommand(
      std::move(cmd_args_),
      [ this, callback = std::move(callback) ](ReplyPtr reply) mutable {
        if (reply->data.IsStatus()) return;
        if (callback) {
          engine::CriticalAsync(sentinel_->task_processor_, std::move(callback),
                                std::move(reply))
              .Detach();
        }
      },
      sentinel_->GetCommandControl(command_control));
  if (first_shard_ == static_cast<size_t>(-1))
    sentinel_->AsyncCommand(command, first_key_, true);
  else
    sentinel_->AsyncCommand(command, true, first_shard_);
}

}  // namespace redis
}  // namespace storages
