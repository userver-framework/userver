#pragma once

#include <inttypes.h>
#include <atomic>
#include <chrono>
#include <memory>
#include <string>
#include <vector>

#include <engine/ev/impl_in_ev_loop.hpp>
#include <engine/ev/thread_pool.hpp>
#include <storages/secdist/secdist.hpp>

#include "base.hpp"
#include "key_shard.hpp"
#include "request.hpp"

namespace storages {
namespace redis {

class SentinelImpl;
class Transaction;

class Sentinel : public std::enable_shared_from_this<Sentinel>,
                 engine::ev::ImplInEvLoop<SentinelImpl> {
 public:
  using ReadyChangeCallback = std::function<void(
      size_t shard, const std::string& shard_name, bool master, bool ready)>;

  Sentinel(const engine::ev::ThreadControl& sentinel_thread_control,
           engine::ev::ThreadPool& thread_pool,
           engine::TaskProcessor& task_processor,
           const std::vector<std::string>& shards,
           const std::vector<ConnectionInfo>& conns,
           const std::string& password, ReadyChangeCallback ready_callback,
           std::unique_ptr<KeyShard>&& key_shard = nullptr,
           CommandControl command_control = command_control_init,
           bool track_masters = true, bool track_slaves = true);
  ~Sentinel();

  static std::shared_ptr<redis::Sentinel> CreateSentinel(
      const secdist::RedisSettings& settings,
      const engine::ev::ThreadControl& sentinel_thread_control,
      engine::ev::ThreadPool& thread_pool,
      engine::TaskProcessor& task_processor);
  static std::shared_ptr<redis::Sentinel> CreateSentinel(
      const secdist::RedisSettings& settings,
      const engine::ev::ThreadControl& sentinel_thread_control,
      engine::ev::ThreadPool& thread_pool,
      engine::TaskProcessor& task_processor,
      ReadyChangeCallback&& ready_callback);

  size_t ShardByKey(const std::string& key) const;
  size_t ShardCount() const;
  void AsyncCommand(CommandPtr command, bool master = true, size_t shard = 0);
  void AsyncCommand(CommandPtr command, const std::string& key,
                    bool master = true);
  Stat GetStat();

  Request MakeRequest(CmdArgs&& args, const std::string& key,
                      bool master = true,
                      const CommandControl& command_control = CommandControl(),
                      bool skip_status = false);
  Request MakeRequest(CmdArgs&& args, size_t shard, bool master = true,
                      const CommandControl& command_control = CommandControl(),
                      bool skip_status = false);

  Request Append(const std::string& key, const std::string& value,
                 const CommandControl& command_control = CommandControl());
  void Append(const std::string& key, const std::string& value,
              Callback&& callback,
              const CommandControl& command_control = CommandControl());
  Request Bitcount(const std::string& key,
                   const CommandControl& command_control = CommandControl());
  void Bitcount(const std::string& key, Callback&& callback,
                const CommandControl& command_control = CommandControl());
  Request Bitcount(const std::string& key, int64_t start, int64_t end,
                   const CommandControl& command_control = CommandControl());
  void Bitcount(const std::string& key, int64_t start, int64_t end,
                Callback&& callback,
                const CommandControl& command_control = CommandControl());
  Request Bitop(const std::string& operation, const std::string& destkey,
                const std::string& key,
                const CommandControl& command_control = CommandControl());
  void Bitop(const std::string& operation, const std::string& destkey,
             const std::string& key, Callback&& callback,
             const CommandControl& command_control = CommandControl());
  Request Bitop(const std::string& operation, const std::string& destkey,
                const std::vector<std::string>& keys,
                const CommandControl& command_control = CommandControl());
  void Bitop(const std::string& operation, const std::string& destkey,
             const std::vector<std::string>& keys, Callback&& callback,
             const CommandControl& command_control = CommandControl());
  Request Bitpos(const std::string& key, int bit,
                 const CommandControl& command_control = CommandControl());
  void Bitpos(const std::string& key, int bit, Callback&& callback,
              const CommandControl& command_control = CommandControl());
  Request Bitpos(const std::string& key, int bit, int64_t start,
                 const CommandControl& command_control = CommandControl());
  void Bitpos(const std::string& key, int bit, int64_t start,
              Callback&& callback,
              const CommandControl& command_control = CommandControl());
  Request Bitpos(const std::string& key, int bit, int64_t start, int64_t end,
                 const CommandControl& command_control = CommandControl());
  void Bitpos(const std::string& key, int bit, int64_t start, int64_t end,
              Callback&& callback,
              const CommandControl& command_control = CommandControl());
  Request Blpop(const std::string& key, int64_t timeout,
                const CommandControl& command_control = CommandControl());
  void Blpop(const std::string& key, int64_t timeout, Callback&& callback,
             const CommandControl& command_control = CommandControl());
  Request Blpop(const std::vector<std::string>& keys, int64_t timeout,
                const CommandControl& command_control = CommandControl());
  void Blpop(const std::vector<std::string>& keys, int64_t timeout,
             Callback&& callback,
             const CommandControl& command_control = CommandControl());
  Request Brpop(const std::string& key, int64_t timeout,
                const CommandControl& command_control = CommandControl());
  void Brpop(const std::string& key, int64_t timeout, Callback&& callback,
             const CommandControl& command_control = CommandControl());
  Request Brpop(const std::vector<std::string>& keys, int64_t timeout,
                const CommandControl& command_control = CommandControl());
  void Brpop(const std::vector<std::string>& keys, int64_t timeout,
             Callback&& callback,
             const CommandControl& command_control = CommandControl());
  Request ClusterKeyslot(
      const std::string& key,
      const CommandControl& command_control = CommandControl());
  void ClusterKeyslot(const std::string& key, Callback&& callback,
                      const CommandControl& command_control = CommandControl());
  Request Decr(const std::string& key,
               const CommandControl& command_control = CommandControl());
  void Decr(const std::string& key, Callback&& callback,
            const CommandControl& command_control = CommandControl());
  Request Decrby(const std::string& key, int64_t val,
                 const CommandControl& command_control = CommandControl());
  void Decrby(const std::string& key, int64_t val, Callback&& callback,
              const CommandControl& command_control = CommandControl());
  Request Del(const std::string& key,
              const CommandControl& command_control = CommandControl());
  void Del(const std::string& key, Callback&& callback,
           const CommandControl& command_control = CommandControl());
  Request Del(const std::vector<std::string>& keys,
              const CommandControl& command_control = CommandControl());
  void Del(const std::vector<std::string>& keys, Callback&& callback,
           const CommandControl& command_control = CommandControl());
  Request Dump(const std::string& key,
               const CommandControl& command_control = CommandControl());
  void Dump(const std::string& key, Callback&& callback,
            const CommandControl& command_control = CommandControl());
  Request Eval(const std::string& script, int64_t numkeys,
               const std::vector<std::string>& keys,
               const std::vector<std::string>& args,
               const CommandControl& command_control = CommandControl());
  void Eval(const std::string& script, int64_t numkeys,
            const std::vector<std::string>& keys,
            const std::vector<std::string>& args, Callback&& callback,
            const CommandControl& command_control = CommandControl());
  Request Evalsha(const std::string& sha1, int64_t numkeys,
                  const std::vector<std::string>& keys,
                  const std::vector<std::string>& args,
                  const CommandControl& command_control = CommandControl());
  void Evalsha(const std::string& sha1, int64_t numkeys,
               const std::vector<std::string>& keys,
               const std::vector<std::string>& args, Callback&& callback,
               const CommandControl& command_control = CommandControl());
  Request Exists(const std::string& key,
                 const CommandControl& command_control = CommandControl());
  void Exists(const std::string& key, Callback&& callback,
              const CommandControl& command_control = CommandControl());
  Request Exists(const std::vector<std::string>& keys,
                 const CommandControl& command_control = CommandControl());
  void Exists(const std::vector<std::string>& keys, Callback&& callback,
              const CommandControl& command_control = CommandControl());
  Request Expire(const std::string& key, std::chrono::seconds seconds,
                 const CommandControl& command_control = CommandControl());
  void Expire(const std::string& key, std::chrono::seconds seconds,
              Callback&& callback,
              const CommandControl& command_control = CommandControl());
  Request Expireat(const std::string& key, int64_t timestamp,
                   const CommandControl& command_control = CommandControl());
  void Expireat(const std::string& key, int64_t timestamp, Callback&& callback,
                const CommandControl& command_control = CommandControl());
  Request Geoadd(const std::string& key, const GeoaddArg& lon_lat_member,
                 const CommandControl& command_control = CommandControl());
  void Geoadd(const std::string& key, const GeoaddArg& lon_lat_member,
              Callback&& callback,
              const CommandControl& command_control = CommandControl());
  Request Geoadd(const std::string& key,
                 const std::vector<GeoaddArg>& lon_lat_member,
                 const CommandControl& command_control = CommandControl());
  void Geoadd(const std::string& key,
              const std::vector<GeoaddArg>& lon_lat_member, Callback&& callback,
              const CommandControl& command_control = CommandControl());
  Request Geodist(const std::string& key, const std::string& member_1,
                  const std::string& member_2,
                  const CommandControl& command_control = CommandControl());
  void Geodist(const std::string& key, const std::string& member_1,
               const std::string& member_2, Callback&& callback,
               const CommandControl& command_control = CommandControl());
  Request Geodist(const std::string& key, const std::string& member_1,
                  const std::string& member_2, const std::string& unit,
                  const CommandControl& command_control = CommandControl());
  void Geodist(const std::string& key, const std::string& member_1,
               const std::string& member_2, const std::string& unit,
               Callback&& callback,
               const CommandControl& command_control = CommandControl());
  Request Geohash(const std::string& key, const std::string& member,
                  const CommandControl& command_control = CommandControl());
  void Geohash(const std::string& key, const std::string& member,
               Callback&& callback,
               const CommandControl& command_control = CommandControl());
  Request Geohash(const std::string& key,
                  const std::vector<std::string>& members,
                  const CommandControl& command_control = CommandControl());
  void Geohash(const std::string& key, const std::vector<std::string>& members,
               Callback&& callback,
               const CommandControl& command_control = CommandControl());
  Request Geopos(const std::string& key, const std::string& member,
                 const CommandControl& command_control = CommandControl());
  void Geopos(const std::string& key, const std::string& member,
              Callback&& callback,
              const CommandControl& command_control = CommandControl());
  Request Geopos(const std::string& key,
                 const std::vector<std::string>& members,
                 const CommandControl& command_control = CommandControl());
  void Geopos(const std::string& key, const std::vector<std::string>& members,
              Callback&& callback,
              const CommandControl& command_control = CommandControl());
  Request Georadius(const std::string& key, double lon, double lat,
                    double radius, const GeoradiusOptions& options,
                    const CommandControl& command_control = CommandControl());
  void Georadius(const std::string& key, double lon, double lat, double radius,
                 const GeoradiusOptions& options, Callback&& callback,
                 const CommandControl& command_control = CommandControl());
  Request Georadius(const std::string& key, double lon, double lat,
                    double radius, const std::string& unit,
                    const GeoradiusOptions& options,
                    const CommandControl& command_control = CommandControl());
  void Georadius(const std::string& key, double lon, double lat, double radius,
                 const std::string& unit, const GeoradiusOptions& options,
                 Callback&& callback,
                 const CommandControl& command_control = CommandControl());
  Request Georadiusbymember(
      const std::string& key, const std::string& member, double radius,
      const GeoradiusOptions& options,
      const CommandControl& command_control = CommandControl());
  void Georadiusbymember(
      const std::string& key, const std::string& member, double radius,
      const GeoradiusOptions& options, Callback&& callback,
      const CommandControl& command_control = CommandControl());
  Request Georadiusbymember(
      const std::string& key, const std::string& member, double radius,
      const std::string& unit, const GeoradiusOptions& options,
      const CommandControl& command_control = CommandControl());
  void Georadiusbymember(
      const std::string& key, const std::string& member, double radius,
      const std::string& unit, const GeoradiusOptions& options,
      Callback&& callback,
      const CommandControl& command_control = CommandControl());
  Request Get(const std::string& key,
              const CommandControl& command_control = CommandControl());
  void Get(const std::string& key, Callback&& callback,
           const CommandControl& command_control = CommandControl());
  Request Getbit(const std::string& key, int64_t offset,
                 const CommandControl& command_control = CommandControl());
  void Getbit(const std::string& key, int64_t offset, Callback&& callback,
              const CommandControl& command_control = CommandControl());
  Request Getrange(const std::string& key, int64_t start, int64_t end,
                   const CommandControl& command_control = CommandControl());
  void Getrange(const std::string& key, int64_t start, int64_t end,
                Callback&& callback,
                const CommandControl& command_control = CommandControl());
  Request Getset(const std::string& key, const std::string& val,
                 const CommandControl& command_control = CommandControl());
  void Getset(const std::string& key, const std::string& val,
              Callback&& callback,
              const CommandControl& command_control = CommandControl());
  Request Hdel(const std::string& key, const std::string& field,
               const CommandControl& command_control = CommandControl());
  void Hdel(const std::string& key, const std::string& field,
            Callback&& callback,
            const CommandControl& command_control = CommandControl());
  Request Hdel(const std::string& key, const std::vector<std::string>& fields,
               const CommandControl& command_control = CommandControl());
  void Hdel(const std::string& key, const std::vector<std::string>& fields,
            Callback&& callback,
            const CommandControl& command_control = CommandControl());
  Request Hexists(const std::string& key, const std::string& field,
                  const CommandControl& command_control = CommandControl());
  void Hexists(const std::string& key, const std::string& field,
               Callback&& callback,
               const CommandControl& command_control = CommandControl());
  Request Hget(const std::string& key, const std::string& field,
               const CommandControl& command_control = CommandControl());
  void Hget(const std::string& key, const std::string& field,
            Callback&& callback,
            const CommandControl& command_control = CommandControl());
  Request Hgetall(const std::string& key,
                  const CommandControl& command_control = CommandControl());
  void Hgetall(const std::string& key, Callback&& callback,
               const CommandControl& command_control = CommandControl());
  Request Hincrby(const std::string& key, const std::string& field,
                  int64_t incr,
                  const CommandControl& command_control = CommandControl());
  void Hincrby(const std::string& key, const std::string& field, int64_t incr,
               Callback&& callback,
               const CommandControl& command_control = CommandControl());
  Request Hincrbyfloat(
      const std::string& key, const std::string& field, double incr,
      const CommandControl& command_control = CommandControl());
  void Hincrbyfloat(const std::string& key, const std::string& field,
                    double incr, Callback&& callback,
                    const CommandControl& command_control = CommandControl());
  Request Hkeys(const std::string& key,
                const CommandControl& command_control = CommandControl());
  void Hkeys(const std::string& key, Callback&& callback,
             const CommandControl& command_control = CommandControl());
  Request Hlen(const std::string& key,
               const CommandControl& command_control = CommandControl());
  void Hlen(const std::string& key, Callback&& callback,
            const CommandControl& command_control = CommandControl());
  Request Hmget(const std::string& key, const std::vector<std::string>& fields,
                const CommandControl& command_control = CommandControl());
  void Hmget(const std::string& key, const std::vector<std::string>& fields,
             Callback&& callback,
             const CommandControl& command_control = CommandControl());
  Request Hmset(
      const std::string& key,
      const std::vector<std::pair<std::string, std::string>>& field_val,
      const CommandControl& command_control = CommandControl());
  void Hmset(const std::string& key,
             const std::vector<std::pair<std::string, std::string>>& field_val,
             Callback&& callback,
             const CommandControl& command_control = CommandControl());
  Request Hset(const std::string& key, const std::string& field,
               const std::string& value,
               const CommandControl& command_control = CommandControl());
  void Hset(const std::string& key, const std::string& field,
            const std::string& value, Callback&& callback,
            const CommandControl& command_control = CommandControl());
  Request Hsetnx(const std::string& key, const std::string& field,
                 const std::string& value,
                 const CommandControl& command_control = CommandControl());
  void Hsetnx(const std::string& key, const std::string& field,
              const std::string& value, Callback&& callback,
              const CommandControl& command_control = CommandControl());
  Request Hstrlen(const std::string& key, const std::string& field,
                  const CommandControl& command_control = CommandControl());
  void Hstrlen(const std::string& key, const std::string& field,
               Callback&& callback,
               const CommandControl& command_control = CommandControl());
  Request Hvals(const std::string& key,
                const CommandControl& command_control = CommandControl());
  void Hvals(const std::string& key, Callback&& callback,
             const CommandControl& command_control = CommandControl());
  Request Incr(const std::string& key,
               const CommandControl& command_control = CommandControl());
  void Incr(const std::string& key, Callback&& callback,
            const CommandControl& command_control = CommandControl());
  Request Incrby(const std::string& key, int64_t incr,
                 const CommandControl& command_control = CommandControl());
  void Incrby(const std::string& key, int64_t incr, Callback&& callback,
              const CommandControl& command_control = CommandControl());
  Request Incrbyfloat(const std::string& key, double incr,
                      const CommandControl& command_control = CommandControl());
  void Incrbyfloat(const std::string& key, double incr, Callback&& callback,
                   const CommandControl& command_control = CommandControl());
  Request Keys(const std::string& keys_pattern, size_t shard,
               const CommandControl& command_control = CommandControl());
  void Keys(const std::string& keys_pattern, size_t shard, Callback&& callback,
            const CommandControl& command_control = CommandControl());
  Request Lindex(const std::string& key, int64_t index,
                 const CommandControl& command_control = CommandControl());
  void Lindex(const std::string& key, int64_t index, Callback&& callback,
              const CommandControl& command_control = CommandControl());
  Request Linsert(const std::string& key, const std::string& before_after,
                  const std::string& pivot, const std::string& value,
                  const CommandControl& command_control = CommandControl());
  void Linsert(const std::string& key, const std::string& before_after,
               const std::string& pivot, const std::string& value,
               Callback&& callback,
               const CommandControl& command_control = CommandControl());
  Request Llen(const std::string& key,
               const CommandControl& command_control = CommandControl());
  void Llen(const std::string& key, Callback&& callback,
            const CommandControl& command_control = CommandControl());
  Request Lpop(const std::string& key,
               const CommandControl& command_control = CommandControl());
  void Lpop(const std::string& key, Callback&& callback,
            const CommandControl& command_control = CommandControl());
  Request Lpush(const std::string& key, const std::string& value,
                const CommandControl& command_control = CommandControl());
  void Lpush(const std::string& key, const std::string& value,
             Callback&& callback,
             const CommandControl& command_control = CommandControl());
  Request Lpush(const std::string& key, const std::vector<std::string>& values,
                const CommandControl& command_control = CommandControl());
  void Lpush(const std::string& key, const std::vector<std::string>& values,
             Callback&& callback,
             const CommandControl& command_control = CommandControl());
  Request Lpushx(const std::string& key, const std::string& value,
                 const CommandControl& command_control = CommandControl());
  void Lpushx(const std::string& key, const std::string& value,
              Callback&& callback,
              const CommandControl& command_control = CommandControl());
  Request Lrange(const std::string& key, int64_t start, int64_t stop,
                 const CommandControl& command_control = CommandControl());
  void Lrange(const std::string& key, int64_t start, int64_t stop,
              Callback&& callback,
              const CommandControl& command_control = CommandControl());
  Request Lrem(const std::string& key, int64_t count, const std::string& value,
               const CommandControl& command_control = CommandControl());
  void Lrem(const std::string& key, int64_t count, const std::string& value,
            Callback&& callback,
            const CommandControl& command_control = CommandControl());
  Request Lset(const std::string& key, int64_t index, const std::string& value,
               const CommandControl& command_control = CommandControl());
  void Lset(const std::string& key, int64_t index, const std::string& value,
            Callback&& callback,
            const CommandControl& command_control = CommandControl());
  Request Ltrim(const std::string& key, int64_t start, int64_t stop,
                const CommandControl& command_control = CommandControl());
  void Ltrim(const std::string& key, int64_t start, int64_t stop,
             Callback&& callback,
             const CommandControl& command_control = CommandControl());
  Request Mget(const std::vector<std::string>& keys,
               const CommandControl& command_control = CommandControl());
  void Mget(const std::vector<std::string>& keys, Callback&& callback,
            const CommandControl& command_control = CommandControl());
  Request Persist(const std::string& key,
                  const CommandControl& command_control = CommandControl());
  void Persist(const std::string& key, Callback&& callback,
               const CommandControl& command_control = CommandControl());
  Request Pexpire(const std::string& key,
                  std::chrono::milliseconds milliseconds,
                  const CommandControl& command_control = CommandControl());
  void Pexpire(const std::string& key, std::chrono::milliseconds milliseconds,
               Callback&& callback,
               const CommandControl& command_control = CommandControl());
  Request Pexpireat(const std::string& key, int64_t milliseconds_timestamp,
                    const CommandControl& command_control = CommandControl());
  void Pexpireat(const std::string& key, int64_t milliseconds_timestamp,
                 Callback&& callback,
                 const CommandControl& command_control = CommandControl());
  Request Pfadd(const std::string& key, const std::string& element,
                const CommandControl& command_control = CommandControl());
  void Pfadd(const std::string& key, const std::string& element,
             Callback&& callback,
             const CommandControl& command_control = CommandControl());
  Request Pfadd(const std::string& key,
                const std::vector<std::string>& elements,
                const CommandControl& command_control = CommandControl());
  void Pfadd(const std::string& key, const std::vector<std::string>& elements,
             Callback&& callback,
             const CommandControl& command_control = CommandControl());
  Request Pfcount(const std::string& key,
                  const CommandControl& command_control = CommandControl());
  void Pfcount(const std::string& key, Callback&& callback,
               const CommandControl& command_control = CommandControl());
  Request Pfcount(const std::vector<std::string>& keys,
                  const CommandControl& command_control = CommandControl());
  void Pfcount(const std::vector<std::string>& keys, Callback&& callback,
               const CommandControl& command_control = CommandControl());
  Request Pfmerge(const std::string& key,
                  const std::vector<std::string>& sourcekeys,
                  const CommandControl& command_control = CommandControl());
  void Pfmerge(const std::string& key,
               const std::vector<std::string>& sourcekeys, Callback&& callback,
               const CommandControl& command_control = CommandControl());
  Request Psetex(const std::string& key, std::chrono::milliseconds milliseconds,
                 const std::string& val,
                 const CommandControl& command_control = CommandControl());
  void Psetex(const std::string& key, std::chrono::milliseconds milliseconds,
              const std::string& val, Callback&& callback,
              const CommandControl& command_control = CommandControl());
  Request Pttl(const std::string& key,
               const CommandControl& command_control = CommandControl());
  void Pttl(const std::string& key, Callback&& callback,
            const CommandControl& command_control = CommandControl());
  Request Restore(const std::string& key, int64_t ttl,
                  const std::string& serialized_value,
                  const CommandControl& command_control = CommandControl());
  void Restore(const std::string& key, int64_t ttl,
               const std::string& serialized_value, Callback&& callback,
               const CommandControl& command_control = CommandControl());
  Request Restore(const std::string& key, int64_t ttl,
                  const std::string& serialized_value,
                  const std::string& replace,
                  const CommandControl& command_control = CommandControl());
  void Restore(const std::string& key, int64_t ttl,
               const std::string& serialized_value, const std::string& replace,
               Callback&& callback,
               const CommandControl& command_control = CommandControl());
  Request Rpop(const std::string& key,
               const CommandControl& command_control = CommandControl());
  void Rpop(const std::string& key, Callback&& callback,
            const CommandControl& command_control = CommandControl());
  Request Rpoplpush(const std::string& key, const std::string& destination,
                    const CommandControl& command_control = CommandControl());
  void Rpoplpush(const std::string& key, const std::string& destination,
                 Callback&& callback,
                 const CommandControl& command_control = CommandControl());
  Request Rpush(const std::string& key, const std::string& value,
                const CommandControl& command_control = CommandControl());
  void Rpush(const std::string& key, const std::string& value,
             Callback&& callback,
             const CommandControl& command_control = CommandControl());
  Request Rpush(const std::string& key, const std::vector<std::string>& values,
                const CommandControl& command_control = CommandControl());
  void Rpush(const std::string& key, const std::vector<std::string>& values,
             Callback&& callback,
             const CommandControl& command_control = CommandControl());
  Request Rpushx(const std::string& key, const std::string& value,
                 const CommandControl& command_control = CommandControl());
  void Rpushx(const std::string& key, const std::string& value,
              Callback&& callback,
              const CommandControl& command_control = CommandControl());
  Request Sadd(const std::string& key, const std::string& member,
               const CommandControl& command_control = CommandControl());
  void Sadd(const std::string& key, const std::string& member,
            Callback&& callback,
            const CommandControl& command_control = CommandControl());
  Request Sadd(const std::string& key, const std::vector<std::string>& members,
               const CommandControl& command_control = CommandControl());
  void Sadd(const std::string& key, const std::vector<std::string>& members,
            Callback&& callback,
            const CommandControl& command_control = CommandControl());
  Request Scard(const std::string& key,
                const CommandControl& command_control = CommandControl());
  void Scard(const std::string& key, Callback&& callback,
             const CommandControl& command_control = CommandControl());
  Request Sdiff(const std::vector<std::string>& keys,
                const CommandControl& command_control = CommandControl());
  void Sdiff(const std::vector<std::string>& keys, Callback&& callback,
             const CommandControl& command_control = CommandControl());
  Request Sdiffstore(const std::string& destination,
                     const std::vector<std::string>& keys,
                     const CommandControl& command_control = CommandControl());
  void Sdiffstore(const std::string& destination,
                  const std::vector<std::string>& keys, Callback&& callback,
                  const CommandControl& command_control = CommandControl());
  Request Set(const std::string& key, const std::string& value,
              const CommandControl& command_control = CommandControl());
  void Set(const std::string& key, const std::string& value,
           Callback&& callback,
           const CommandControl& command_control = CommandControl());
  Request Set(const std::string& key, const std::string& value,
              const SetOptions& options,
              const CommandControl& command_control = CommandControl());
  void Set(const std::string& key, const std::string& value,
           const SetOptions& options, Callback&& callback,
           const CommandControl& command_control = CommandControl());
  Request Setbit(const std::string& key, int64_t offset,
                 const std::string& value,
                 const CommandControl& command_control = CommandControl());
  void Setbit(const std::string& key, int64_t offset, const std::string& value,
              Callback&& callback,
              const CommandControl& command_control = CommandControl());
  Request Setex(const std::string& key, std::chrono::seconds seconds,
                const std::string& value,
                const CommandControl& command_control = CommandControl());
  void Setex(const std::string& key, std::chrono::seconds seconds,
             const std::string& value, Callback&& callback,
             const CommandControl& command_control = CommandControl());
  Request Setnx(const std::string& key, const std::string& value,
                const CommandControl& command_control = CommandControl());
  void Setnx(const std::string& key, const std::string& value,
             Callback&& callback,
             const CommandControl& command_control = CommandControl());
  Request Setrange(const std::string& key, int64_t offset,
                   const std::string& value,
                   const CommandControl& command_control = CommandControl());
  void Setrange(const std::string& key, int64_t offset,
                const std::string& value, Callback&& callback,
                const CommandControl& command_control = CommandControl());
  Request Sinter(const std::vector<std::string>& keys,
                 const CommandControl& command_control = CommandControl());
  void Sinter(const std::vector<std::string>& keys, Callback&& callback,
              const CommandControl& command_control = CommandControl());
  Request Sinterstore(const std::string& destination,
                      const std::vector<std::string>& keys,
                      const CommandControl& command_control = CommandControl());
  void Sinterstore(const std::string& destination,
                   const std::vector<std::string>& keys, Callback&& callback,
                   const CommandControl& command_control = CommandControl());
  Request Sismember(const std::string& key, const std::string& member,
                    const CommandControl& command_control = CommandControl());
  void Sismember(const std::string& key, const std::string& member,
                 Callback&& callback,
                 const CommandControl& command_control = CommandControl());
  Request Smembers(const std::string& key,
                   const CommandControl& command_control = CommandControl());
  void Smembers(const std::string& key, Callback&& callback,
                const CommandControl& command_control = CommandControl());
  Request Smove(const std::string& key, const std::string& destination,
                const std::string& member,
                const CommandControl& command_control = CommandControl());
  void Smove(const std::string& key, const std::string& destination,
             const std::string& member, Callback&& callback,
             const CommandControl& command_control = CommandControl());
  Request Spop(const std::string& key,
               const CommandControl& command_control = CommandControl());
  void Spop(const std::string& key, Callback&& callback,
            const CommandControl& command_control = CommandControl());
  Request Spop(const std::string& key, int64_t count,
               const CommandControl& command_control = CommandControl());
  void Spop(const std::string& key, int64_t count, Callback&& callback,
            const CommandControl& command_control = CommandControl());
  Request Srandmember(const std::string& key,
                      const CommandControl& command_control = CommandControl());
  void Srandmember(const std::string& key, Callback&& callback,
                   const CommandControl& command_control = CommandControl());
  Request Srandmember(const std::string& key, int64_t count,
                      const CommandControl& command_control = CommandControl());
  void Srandmember(const std::string& key, int64_t count, Callback&& callback,
                   const CommandControl& command_control = CommandControl());
  Request Srem(const std::string& key, const std::string& members,
               const CommandControl& command_control = CommandControl());
  void Srem(const std::string& key, const std::string& members,
            Callback&& callback,
            const CommandControl& command_control = CommandControl());
  Request Srem(const std::string& key, const std::vector<std::string>& members,
               const CommandControl& command_control = CommandControl());
  void Srem(const std::string& key, const std::vector<std::string>& members,
            Callback&& callback,
            const CommandControl& command_control = CommandControl());
  Request Strlen(const std::string& key,
                 const CommandControl& command_control = CommandControl());
  void Strlen(const std::string& key, Callback&& callback,
              const CommandControl& command_control = CommandControl());
  Request Sunion(const std::vector<std::string>& keys,
                 const CommandControl& command_control = CommandControl());
  void Sunion(const std::vector<std::string>& keys, Callback&& callback,
              const CommandControl& command_control = CommandControl());
  Request Sunionstore(const std::string& destination,
                      const std::vector<std::string>& keys,
                      const CommandControl& command_control = CommandControl());
  void Sunionstore(const std::string& destination,
                   const std::vector<std::string>& keys, Callback&& callback,
                   const CommandControl& command_control = CommandControl());
  Request Ttl(const std::string& key,
              const CommandControl& command_control = CommandControl());
  void Ttl(const std::string& key, Callback&& callback,
           const CommandControl& command_control = CommandControl());
  Request Type(const std::string& key,
               const CommandControl& command_control = CommandControl());
  void Type(const std::string& key, Callback&& callback,
            const CommandControl& command_control = CommandControl());
  Request Zadd(const std::string& key, double score, const std::string& member,
               const CommandControl& command_control = CommandControl());
  void Zadd(const std::string& key, double score, const std::string& member,
            Callback&& callback,
            const CommandControl& command_control = CommandControl());
  Request Zcard(const std::string& key,
                const CommandControl& command_control = CommandControl());
  void Zcard(const std::string& key, Callback&& callback,
             const CommandControl& command_control = CommandControl());
  Request Zcount(const std::string& key, int64_t min, int64_t max,
                 const CommandControl& command_control = CommandControl());
  void Zcount(const std::string& key, int64_t min, int64_t max,
              Callback&& callback,
              const CommandControl& command_control = CommandControl());
  Request Zcount(const std::string& key, double min, double max,
                 const CommandControl& command_control = CommandControl());
  void Zcount(const std::string& key, double min, double max,
              Callback&& callback,
              const CommandControl& command_control = CommandControl());
  Request Zcount(const std::string& key, const std::string& min,
                 const std::string& max,
                 const CommandControl& command_control = CommandControl());
  void Zcount(const std::string& key, const std::string& min,
              const std::string& max, Callback&& callback,
              const CommandControl& command_control = CommandControl());
  Request Zincrby(const std::string& key, int64_t incr,
                  const std::string& member,
                  const CommandControl& command_control = CommandControl());
  void Zincrby(const std::string& key, int64_t incr, const std::string& member,
               Callback&& callback,
               const CommandControl& command_control = CommandControl());
  Request Zincrby(const std::string& key, double incr,
                  const std::string& member,
                  const CommandControl& command_control = CommandControl());
  void Zincrby(const std::string& key, double incr, const std::string& member,
               Callback&& callback,
               const CommandControl& command_control = CommandControl());
  Request Zincrby(const std::string& key, const std::string& incr,
                  const std::string& member,
                  const CommandControl& command_control = CommandControl());
  void Zincrby(const std::string& key, const std::string& incr,
               const std::string& member, Callback&& callback,
               const CommandControl& command_control = CommandControl());
  Request Zlexcount(const std::string& key, int64_t min, int64_t max,
                    const CommandControl& command_control = CommandControl());
  void Zlexcount(const std::string& key, int64_t min, int64_t max,
                 Callback&& callback,
                 const CommandControl& command_control = CommandControl());
  Request Zlexcount(const std::string& key, double min, double max,
                    const CommandControl& command_control = CommandControl());
  void Zlexcount(const std::string& key, double min, double max,
                 Callback&& callback,
                 const CommandControl& command_control = CommandControl());
  Request Zlexcount(const std::string& key, const std::string& min,
                    const std::string& max,
                    const CommandControl& command_control = CommandControl());
  void Zlexcount(const std::string& key, const std::string& min,
                 const std::string& max, Callback&& callback,
                 const CommandControl& command_control = CommandControl());
  Request Zrange(const std::string& key, int64_t start, int64_t stop,
                 const ScoreOptions& score_options,
                 const CommandControl& command_control = CommandControl());
  void Zrange(const std::string& key, int64_t start, int64_t stop,
              const ScoreOptions& score_options, Callback&& callback,
              const CommandControl& command_control = CommandControl());
  Request Zrange(const std::string& key, double start, double stop,
                 const ScoreOptions& score_options,
                 const CommandControl& command_control = CommandControl());
  void Zrange(const std::string& key, double start, double stop,
              const ScoreOptions& score_options, Callback&& callback,
              const CommandControl& command_control = CommandControl());
  Request Zrange(const std::string& key, const std::string& start,
                 const std::string& stop, const ScoreOptions& score_options,
                 const CommandControl& command_control = CommandControl());
  void Zrange(const std::string& key, const std::string& start,
              const std::string& stop, const ScoreOptions& score_options,
              Callback&& callback,
              const CommandControl& command_control = CommandControl());
  Request Zrank(const std::string& key, const std::string& member,
                const CommandControl& command_control = CommandControl());
  void Zrank(const std::string& key, const std::string& member,
             Callback&& callback,
             const CommandControl& command_control = CommandControl());
  Request Zrem(const std::string& key, const std::string& member,
               const CommandControl& command_control = CommandControl());
  void Zrem(const std::string& key, const std::string& member,
            Callback&& callback,
            const CommandControl& command_control = CommandControl());
  Request Zrem(const std::string& key, const std::vector<std::string>& members,
               const CommandControl& command_control = CommandControl());
  void Zrem(const std::string& key, const std::vector<std::string>& members,
            Callback&& callback,
            const CommandControl& command_control = CommandControl());
  Request Zremrangebylex(
      const std::string& key, int64_t min, int64_t max,
      const CommandControl& command_control = CommandControl());
  void Zremrangebylex(const std::string& key, int64_t min, int64_t max,
                      Callback&& callback,
                      const CommandControl& command_control = CommandControl());
  Request Zremrangebylex(
      const std::string& key, double min, double max,
      const CommandControl& command_control = CommandControl());
  void Zremrangebylex(const std::string& key, double min, double max,
                      Callback&& callback,
                      const CommandControl& command_control = CommandControl());
  Request Zremrangebylex(
      const std::string& key, const std::string& min, const std::string& max,
      const CommandControl& command_control = CommandControl());
  void Zremrangebylex(const std::string& key, const std::string& min,
                      const std::string& max, Callback&& callback,
                      const CommandControl& command_control = CommandControl());
  Request Zremrangebyrank(
      const std::string& key, int64_t start, int64_t stop,
      const CommandControl& command_control = CommandControl());
  void Zremrangebyrank(
      const std::string& key, int64_t start, int64_t stop, Callback&& callback,
      const CommandControl& command_control = CommandControl());
  Request Zremrangebyrank(
      const std::string& key, double start, double stop,
      const CommandControl& command_control = CommandControl());
  void Zremrangebyrank(
      const std::string& key, double start, double stop, Callback&& callback,
      const CommandControl& command_control = CommandControl());
  Request Zremrangebyrank(
      const std::string& key, const std::string& start, const std::string& stop,
      const CommandControl& command_control = CommandControl());
  void Zremrangebyrank(
      const std::string& key, const std::string& start, const std::string& stop,
      Callback&& callback,
      const CommandControl& command_control = CommandControl());
  Request Zremrangebyscore(
      const std::string& key, int64_t min, int64_t max,
      const CommandControl& command_control = CommandControl());
  void Zremrangebyscore(
      const std::string& key, int64_t min, int64_t max, Callback&& callback,
      const CommandControl& command_control = CommandControl());
  Request Zremrangebyscore(
      const std::string& key, double min, double max,
      const CommandControl& command_control = CommandControl());
  void Zremrangebyscore(
      const std::string& key, double min, double max, Callback&& callback,
      const CommandControl& command_control = CommandControl());
  Request Zremrangebyscore(
      const std::string& key, const std::string& min, const std::string& max,
      const CommandControl& command_control = CommandControl());
  void Zremrangebyscore(
      const std::string& key, const std::string& min, const std::string& max,
      Callback&& callback,
      const CommandControl& command_control = CommandControl());
  Request Zrevrange(const std::string& key, int64_t start, int64_t stop,
                    const ScoreOptions& score_options,
                    const CommandControl& command_control = CommandControl());
  void Zrevrange(const std::string& key, int64_t start, int64_t stop,
                 const ScoreOptions& score_options, Callback&& callback,
                 const CommandControl& command_control = CommandControl());
  Request Zrevrange(const std::string& key, double start, double stop,
                    const ScoreOptions& score_options,
                    const CommandControl& command_control = CommandControl());
  void Zrevrange(const std::string& key, double start, double stop,
                 const ScoreOptions& score_options, Callback&& callback,
                 const CommandControl& command_control = CommandControl());
  Request Zrevrange(const std::string& key, const std::string& start,
                    const std::string& stop, const ScoreOptions& score_options,
                    const CommandControl& command_control = CommandControl());
  void Zrevrange(const std::string& key, const std::string& start,
                 const std::string& stop, const ScoreOptions& score_options,
                 Callback&& callback,
                 const CommandControl& command_control = CommandControl());
  Request Zrevrank(const std::string& key, const std::string& member,
                   const CommandControl& command_control = CommandControl());
  void Zrevrank(const std::string& key, const std::string& member,
                Callback&& callback,
                const CommandControl& command_control = CommandControl());
  Request Zscore(const std::string& key, const std::string& member,
                 const CommandControl& command_control = CommandControl());
  void Zscore(const std::string& key, const std::string& member,
              Callback&& callback,
              const CommandControl& command_control = CommandControl());

  Transaction Multi();

  Request Randomkey(size_t shard,
                    const CommandControl& command_control = CommandControl());
  void Randomkey(size_t shard, Callback&& callback,
                 const CommandControl& command_control = CommandControl());

  Request Publish(const std::string& channel, const std::string& message,
                  const CommandControl& command_control = CommandControl());

  using MessageCallback = std::function<void(const std::string& channel,
                                             const std::string& message)>;
  using PmessageCallback =
      std::function<void(const std::string& pattern, const std::string& channel,
                         const std::string& message)>;
  using SubscribeCallback =
      std::function<void(const std::string& channel, size_t count)>;
  using UnsubscribeCallback =
      std::function<void(const std::string& channel, size_t count)>;

  void Subscribe(
      const std::string& channel, const MessageCallback& message_callback,
      const SubscribeCallback& subscribe_callback = default_subscribe_callback,
      const UnsubscribeCallback& unsubscribe_callback =
          default_unsubscribe_callback);

  void Subscribe(
      const std::vector<std::string>& channels,
      const MessageCallback& message_callback,
      const SubscribeCallback& subscribe_callback = default_subscribe_callback,
      const UnsubscribeCallback& unsubscribe_callback =
          default_unsubscribe_callback);

  void Psubscribe(
      const std::string& channel, const PmessageCallback& pmessage_callback,
      const SubscribeCallback& subscribe_callback = default_subscribe_callback,
      const UnsubscribeCallback& unsubscribe_callback =
          default_unsubscribe_callback);

  void Psubscribe(
      const std::vector<std::string>& channels,
      const PmessageCallback& pmessage_callback,
      const SubscribeCallback& subscribe_callback = default_subscribe_callback,
      const UnsubscribeCallback& unsubscribe_callback =
          default_unsubscribe_callback);

  void Unsubscribe(const std::string& channel);
  void Unsubscribe(const std::vector<std::string>& channels);
  void Punsubscribe(const std::string& channel);
  void Punsubscribe(const std::vector<std::string>& channels);

 private:
  static void SubscribeCallbackNone(const std::string& /*channel*/,
                                    size_t /*count*/) {}
  static void UnsubscribeCallbackNone(const std::string& /*channel*/,
                                      size_t /*count*/) {}
  static const SubscribeCallback default_subscribe_callback;
  static const UnsubscribeCallback default_unsubscribe_callback;

  static void OnSubscribeReply(const MessageCallback message_callback,
                               const SubscribeCallback subscribe_callback,
                               const UnsubscribeCallback unsubscribe_callback,
                               redis::ReplyPtr reply_ptr);

  static void OnPsubscribeReply(const PmessageCallback pmessage_callback,
                                const SubscribeCallback subscribe_callback,
                                const UnsubscribeCallback unsubscribe_callback,
                                redis::ReplyPtr reply_ptr);

  CommandControl GetCommandControl(const CommandControl& cc);

  friend class Transaction;

  CommandControl default_command_control_;
  engine::TaskProcessor& task_processor_;
};

}  // namespace redis
}  // namespace storages
