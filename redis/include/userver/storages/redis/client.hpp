#pragma once

/// @file userver/storages/redis/client.hpp
/// @brief @copybrief storages::redis::Client

#include <chrono>
#include <memory>
#include <string>

#include <userver/storages/redis/impl/base.hpp>
#include <userver/storages/redis/impl/wait_connected_mode.hpp>

#include <userver/storages/redis/client_fwd.hpp>
#include <userver/storages/redis/command_options.hpp>
#include <userver/storages/redis/request.hpp>
#include <userver/storages/redis/request_eval.hpp>
#include <userver/storages/redis/request_evalsha.hpp>
#include <userver/storages/redis/transaction.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

enum class PubShard {
  kZeroShard,
  kRoundRobin,
};

using RetryNilFromMaster = USERVER_NAMESPACE::redis::RetryNilFromMaster;

inline constexpr RetryNilFromMaster kRetryNilFromMaster{};

/// @ingroup userver_clients
///
/// @brief Redis client.
///
/// Usually retrieved from components::Redis component.
///
/// ## Example usage:
///
/// @snippet storages/redis/client_redistest.cpp  Sample Redis Client usage
class Client {
 public:
  virtual ~Client() = default;

  virtual size_t ShardsCount() const = 0;

  virtual size_t ShardByKey(const std::string& key) const = 0;

  void CheckShardIdx(size_t shard_idx) const;

  virtual const std::string& GetAnyKeyForShard(size_t shard_idx) const = 0;

  virtual std::shared_ptr<Client> GetClientForShard(size_t shard_idx) = 0;

  virtual void WaitConnectedOnce(
      USERVER_NAMESPACE::redis::RedisWaitConnected wait_connected) = 0;

  // redis commands:

  virtual RequestAppend Append(std::string key, std::string value,
                               const CommandControl& command_control) = 0;

  virtual RequestDbsize Dbsize(size_t shard,
                               const CommandControl& command_control) = 0;

  virtual RequestDel Del(std::string key,
                         const CommandControl& command_control) = 0;

  virtual RequestDel Del(std::vector<std::string> keys,
                         const CommandControl& command_control) = 0;

  virtual RequestUnlink Unlink(std::string key,
                               const CommandControl& command_control) = 0;

  virtual RequestUnlink Unlink(std::vector<std::string> keys,
                               const CommandControl& command_control) = 0;

  template <typename ScriptResult, typename ReplyType = ScriptResult>
  RequestEval<ScriptResult, ReplyType> Eval(
      std::string script, std::vector<std::string> keys,
      std::vector<std::string> args, const CommandControl& command_control) {
    return RequestEval<ScriptResult, ReplyType>{EvalCommon(
        std::move(script), std::move(keys), std::move(args), command_control)};
  }
  template <typename ScriptResult, typename ReplyType = ScriptResult>
  RequestEvalSha<ScriptResult, ReplyType> EvalSha(
      std::string script_hash, std::vector<std::string> keys,
      std::vector<std::string> args, const CommandControl& command_control) {
    return RequestEvalSha<ScriptResult, ReplyType>{
        EvalShaCommon(std::move(script_hash), std::move(keys), std::move(args),
                      command_control)};
  }

  virtual RequestScriptLoad ScriptLoad(
      std::string script, size_t shard,
      const CommandControl& command_control) = 0;

  template <typename ScriptInfo, typename ReplyType = std::decay_t<ScriptInfo>>
  RequestEval<std::decay_t<ScriptInfo>, ReplyType> Eval(
      const ScriptInfo& script_info, std::vector<std::string> keys,
      std::vector<std::string> args, const CommandControl& command_control) {
    return RequestEval<std::decay_t<ScriptInfo>, ReplyType>{
        EvalCommon(script_info.GetScript(), std::move(keys), std::move(args),
                   command_control)};
  }

  virtual RequestExists Exists(std::string key,
                               const CommandControl& command_control) = 0;

  virtual RequestExists Exists(std::vector<std::string> keys,
                               const CommandControl& command_control) = 0;

  virtual RequestExpire Expire(std::string key, std::chrono::seconds ttl,
                               const CommandControl& command_control) = 0;

  virtual RequestGeoadd Geoadd(std::string key, GeoaddArg point_member,
                               const CommandControl& command_control) = 0;

  virtual RequestGeoadd Geoadd(std::string key,
                               std::vector<GeoaddArg> point_members,
                               const CommandControl& command_control) = 0;

  virtual RequestGeoradius Georadius(std::string key, Longitude lon,
                                     Latitude lat, double radius,
                                     const GeoradiusOptions& georadius_options,
                                     const CommandControl& command_control) = 0;

  virtual RequestGeosearch Geosearch(std::string key, std::string member,
                                     double radius,
                                     const GeosearchOptions& geosearch_options,
                                     const CommandControl& command_control) = 0;

  virtual RequestGeosearch Geosearch(std::string key, std::string member,
                                     BoxWidth width, BoxHeight height,
                                     const GeosearchOptions& geosearch_options,
                                     const CommandControl& command_control) = 0;

  virtual RequestGeosearch Geosearch(std::string key, Longitude lon,
                                     Latitude lat, double radius,
                                     const GeosearchOptions& geosearch_options,
                                     const CommandControl& command_control) = 0;

  virtual RequestGeosearch Geosearch(std::string key, Longitude lon,
                                     Latitude lat, BoxWidth width,
                                     BoxHeight height,
                                     const GeosearchOptions& geosearch_options,
                                     const CommandControl& command_control) = 0;

  virtual RequestGet Get(std::string key,
                         const CommandControl& command_control) = 0;

  virtual RequestGetset Getset(std::string key, std::string value,
                               const CommandControl& command_control) = 0;

  virtual RequestHdel Hdel(std::string key, std::string field,
                           const CommandControl& command_control) = 0;

  virtual RequestHdel Hdel(std::string key, std::vector<std::string> fields,
                           const CommandControl& command_control) = 0;

  virtual RequestHexists Hexists(std::string key, std::string field,
                                 const CommandControl& command_control) = 0;

  virtual RequestHget Hget(std::string key, std::string field,
                           const CommandControl& command_control) = 0;

  // use Hscan in case of a big hash
  virtual RequestHgetall Hgetall(std::string key,
                                 const CommandControl& command_control) = 0;

  virtual RequestHincrby Hincrby(std::string key, std::string field,
                                 int64_t increment,
                                 const CommandControl& command_control) = 0;

  virtual RequestHincrbyfloat Hincrbyfloat(
      std::string key, std::string field, double increment,
      const CommandControl& command_control) = 0;

  // use Hscan in case of a big hash
  virtual RequestHkeys Hkeys(std::string key,
                             const CommandControl& command_control) = 0;

  virtual RequestHlen Hlen(std::string key,
                           const CommandControl& command_control) = 0;

  virtual RequestHmget Hmget(std::string key, std::vector<std::string> fields,
                             const CommandControl& command_control) = 0;

  virtual RequestHmset Hmset(
      std::string key,
      std::vector<std::pair<std::string, std::string>> field_values,
      const CommandControl& command_control) = 0;

  virtual RequestHscan Hscan(std::string key, HscanOptions options,
                             const CommandControl& command_control) = 0;

  virtual RequestHset Hset(std::string key, std::string field,
                           std::string value,
                           const CommandControl& command_control) = 0;

  virtual RequestHsetnx Hsetnx(std::string key, std::string field,
                               std::string value,
                               const CommandControl& command_control) = 0;

  // use Hscan in case of a big hash
  virtual RequestHvals Hvals(std::string key,
                             const CommandControl& command_control) = 0;

  virtual RequestIncr Incr(std::string key,
                           const CommandControl& command_control) = 0;

  [[deprecated("use Scan")]] virtual RequestKeys Keys(
      std::string keys_pattern, size_t shard,
      const CommandControl& command_control) = 0;

  virtual RequestLindex Lindex(std::string key, int64_t index,
                               const CommandControl& command_control) = 0;

  virtual RequestLlen Llen(std::string key,
                           const CommandControl& command_control) = 0;

  virtual RequestLpop Lpop(std::string key,
                           const CommandControl& command_control) = 0;

  virtual RequestLpush Lpush(std::string key, std::string value,
                             const CommandControl& command_control) = 0;

  virtual RequestLpush Lpush(std::string key, std::vector<std::string> values,
                             const CommandControl& command_control) = 0;

  virtual RequestLpushx Lpushx(std::string key, std::string element,
                               const CommandControl& command_control) = 0;

  virtual RequestLrange Lrange(std::string key, int64_t start, int64_t stop,
                               const CommandControl& command_control) = 0;

  virtual RequestLrem Lrem(std::string key, int64_t count, std::string element,
                           const CommandControl& command_control) = 0;

  virtual RequestLtrim Ltrim(std::string key, int64_t start, int64_t stop,
                             const CommandControl& command_control) = 0;

  virtual RequestMget Mget(std::vector<std::string> keys,
                           const CommandControl& command_control) = 0;

  virtual RequestMset Mset(
      std::vector<std::pair<std::string, std::string>> key_values,
      const CommandControl& command_control) = 0;

  virtual TransactionPtr Multi() = 0;

  virtual TransactionPtr Multi(Transaction::CheckShards check_shards) = 0;

  virtual RequestPersist Persist(std::string key,
                                 const CommandControl& command_control) = 0;

  virtual RequestPexpire Pexpire(std::string key, std::chrono::milliseconds ttl,
                                 const CommandControl& command_control) = 0;

  virtual RequestPing Ping(size_t shard,
                           const CommandControl& command_control) = 0;

  virtual RequestPingMessage Ping(size_t shard, std::string message,
                                  const CommandControl& command_control) = 0;

  virtual void Publish(std::string channel, std::string message,
                       const CommandControl& command_control,
                       PubShard policy) = 0;

  virtual RequestRename Rename(std::string key, std::string new_key,
                               const CommandControl& command_control) = 0;

  virtual RequestRpop Rpop(std::string key,
                           const CommandControl& command_control) = 0;

  virtual RequestRpush Rpush(std::string key, std::string value,
                             const CommandControl& command_control) = 0;

  virtual RequestRpush Rpush(std::string key, std::vector<std::string> values,
                             const CommandControl& command_control) = 0;

  virtual RequestRpushx Rpushx(std::string key, std::string element,
                               const CommandControl& command_control) = 0;

  virtual RequestSadd Sadd(std::string key, std::string member,
                           const CommandControl& command_control) = 0;

  virtual RequestSadd Sadd(std::string key, std::vector<std::string> members,
                           const CommandControl& command_control) = 0;

  virtual RequestScan Scan(size_t shard, ScanOptions options,
                           const CommandControl& command_control) = 0;

  virtual RequestScard Scard(std::string key,
                             const CommandControl& command_control) = 0;

  virtual RequestSet Set(std::string key, std::string value,
                         const CommandControl& command_control) = 0;

  virtual RequestSet Set(std::string key, std::string value,
                         std::chrono::milliseconds ttl,
                         const CommandControl& command_control) = 0;

  virtual RequestSetIfExist SetIfExist(
      std::string key, std::string value,
      const CommandControl& command_control) = 0;

  virtual RequestSetIfExist SetIfExist(
      std::string key, std::string value, std::chrono::milliseconds ttl,
      const CommandControl& command_control) = 0;

  virtual RequestSetIfNotExist SetIfNotExist(
      std::string key, std::string value,
      const CommandControl& command_control) = 0;

  virtual RequestSetIfNotExist SetIfNotExist(
      std::string key, std::string value, std::chrono::milliseconds ttl,
      const CommandControl& command_control) = 0;

  virtual RequestSetex Setex(std::string key, std::chrono::seconds seconds,
                             std::string value,
                             const CommandControl& command_control) = 0;

  virtual RequestSismember Sismember(std::string key, std::string member,
                                     const CommandControl& command_control) = 0;

  // use Sscan in case of a big set
  virtual RequestSmembers Smembers(std::string key,
                                   const CommandControl& command_control) = 0;

  virtual RequestSrandmember Srandmember(
      std::string key, const CommandControl& command_control) = 0;

  virtual RequestSrandmembers Srandmembers(
      std::string key, int64_t count,
      const CommandControl& command_control) = 0;

  virtual RequestSrem Srem(std::string key, std::string member,
                           const CommandControl& command_control) = 0;

  virtual RequestSrem Srem(std::string key, std::vector<std::string> members,
                           const CommandControl& command_control) = 0;

  virtual RequestSscan Sscan(std::string key, SscanOptions options,
                             const CommandControl& command_control) = 0;

  virtual RequestStrlen Strlen(std::string key,
                               const CommandControl& command_control) = 0;

  virtual RequestTime Time(size_t shard,
                           const CommandControl& command_control) = 0;

  virtual RequestTtl Ttl(std::string key,
                         const CommandControl& command_control) = 0;

  virtual RequestType Type(std::string key,
                           const CommandControl& command_control) = 0;

  virtual RequestZadd Zadd(std::string key, double score, std::string member,
                           const CommandControl& command_control) = 0;

  virtual RequestZadd Zadd(std::string key, double score, std::string member,
                           const ZaddOptions& options,
                           const CommandControl& command_control) = 0;

  virtual RequestZadd Zadd(
      std::string key,
      std::vector<std::pair<double, std::string>> scored_members,
      const CommandControl& command_control) = 0;

  virtual RequestZadd Zadd(
      std::string key,
      std::vector<std::pair<double, std::string>> scored_members,
      const ZaddOptions& options, const CommandControl& command_control) = 0;

  virtual RequestZaddIncr ZaddIncr(std::string key, double score,
                                   std::string member,
                                   const CommandControl& command_control) = 0;

  virtual RequestZaddIncrExisting ZaddIncrExisting(
      std::string key, double score, std::string member,
      const CommandControl& command_control) = 0;

  virtual RequestZcard Zcard(std::string key,
                             const CommandControl& command_control) = 0;

  virtual RequestZrange Zrange(std::string key, int64_t start, int64_t stop,
                               const CommandControl& command_control) = 0;

  virtual RequestZrangeWithScores ZrangeWithScores(
      std::string key, int64_t start, int64_t stop,
      const CommandControl& command_control) = 0;

  virtual RequestZrangebyscore Zrangebyscore(
      std::string key, double min, double max,
      const CommandControl& command_control) = 0;

  virtual RequestZrangebyscore Zrangebyscore(
      std::string key, std::string min, std::string max,
      const CommandControl& command_control) = 0;

  virtual RequestZrangebyscore Zrangebyscore(
      std::string key, double min, double max,
      const RangeOptions& range_options,
      const CommandControl& command_control) = 0;

  virtual RequestZrangebyscore Zrangebyscore(
      std::string key, std::string min, std::string max,
      const RangeOptions& range_options,
      const CommandControl& command_control) = 0;

  virtual RequestZrangebyscoreWithScores ZrangebyscoreWithScores(
      std::string key, double min, double max,
      const CommandControl& command_control) = 0;

  virtual RequestZrangebyscoreWithScores ZrangebyscoreWithScores(
      std::string key, std::string min, std::string max,
      const CommandControl& command_control) = 0;

  virtual RequestZrangebyscoreWithScores ZrangebyscoreWithScores(
      std::string key, double min, double max,
      const RangeOptions& range_options,
      const CommandControl& command_control) = 0;

  virtual RequestZrangebyscoreWithScores ZrangebyscoreWithScores(
      std::string key, std::string min, std::string max,
      const RangeOptions& range_options,
      const CommandControl& command_control) = 0;

  virtual RequestZrem Zrem(std::string key, std::string member,
                           const CommandControl& command_control) = 0;

  virtual RequestZrem Zrem(std::string key, std::vector<std::string> members,
                           const CommandControl& command_control) = 0;

  virtual RequestZremrangebyrank Zremrangebyrank(
      std::string key, int64_t start, int64_t stop,
      const CommandControl& command_control) = 0;

  virtual RequestZremrangebyscore Zremrangebyscore(
      std::string key, double min, double max,
      const CommandControl& command_control) = 0;

  virtual RequestZremrangebyscore Zremrangebyscore(
      std::string key, std::string min, std::string max,
      const CommandControl& command_control) = 0;

  virtual RequestZscan Zscan(std::string key, ZscanOptions options,
                             const CommandControl& command_control) = 0;

  virtual RequestZscore Zscore(std::string key, std::string member,
                               const CommandControl& command_control) = 0;

  // end of redis commands

  RequestGet Get(std::string key, RetryNilFromMaster,
                 const CommandControl& command_control);

  RequestHget Hget(std::string key, std::string field, RetryNilFromMaster,
                   const CommandControl& command_control);

  RequestZscore Zscore(std::string key, std::string member, RetryNilFromMaster,
                       const CommandControl& command_control);

  void Publish(std::string channel, std::string message,
               const CommandControl& command_control);

  RequestScan Scan(size_t shard, const CommandControl& command_control);

  RequestHscan Hscan(std::string key, const CommandControl& command_control);

  RequestSscan Sscan(std::string key, const CommandControl& command_control);

  RequestZscan Zscan(std::string key, const CommandControl& command_control);

 protected:
  virtual RequestEvalCommon EvalCommon(
      std::string script, std::vector<std::string> keys,
      std::vector<std::string> args, const CommandControl& command_control) = 0;
  virtual RequestEvalShaCommon EvalShaCommon(
      std::string script_hash, std::vector<std::string> keys,
      std::vector<std::string> args, const CommandControl& command_control) = 0;
};

std::string CreateTmpKey(const std::string& key, std::string prefix);

}  // namespace storages::redis

USERVER_NAMESPACE_END
