#pragma once

#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <userver/storages/redis/impl/base.hpp>
#include <userver/storages/redis/impl/command_options.hpp>
#include <userver/storages/redis/impl/request.hpp>

#include <userver/storages/redis/client.hpp>
#include <userver/storages/redis/transaction.hpp>

#include "scan_reply.hpp"

USERVER_NAMESPACE_BEGIN

namespace redis {
class Sentinel;
}  // namespace redis

namespace storages::redis {

class TransactionImpl;

// NOLINTNEXTLINE(fuchsia-multiple-inheritance)
class ClientImpl final : public Client,
                         public std::enable_shared_from_this<ClientImpl> {
 public:
  explicit ClientImpl(
      std::shared_ptr<USERVER_NAMESPACE::redis::Sentinel> sentinel,
      std::optional<size_t> force_shard_idx = std::nullopt);

  void WaitConnectedOnce(
      USERVER_NAMESPACE::redis::RedisWaitConnected wait_connected) override;

  size_t ShardsCount() const override;

  size_t ShardByKey(const std::string& key) const override;

  const std::string& GetAnyKeyForShard(size_t shard_idx) const override;

  std::shared_ptr<Client> GetClientForShard(size_t shard_idx) override;

  std::optional<size_t> GetForcedShardIdx() const;

  Request<ScanReplyTmpl<ScanTag::kScan>> MakeScanRequestNoKey(
      size_t shard, ScanReply::Cursor cursor, ScanOptions options,
      const CommandControl& command_control);

  template <ScanTag scan_tag>
  Request<ScanReplyTmpl<scan_tag>> MakeScanRequestWithKey(
      std::string key, size_t shard,
      typename ScanReplyTmpl<scan_tag>::Cursor cursor,
      ScanOptionsTmpl<scan_tag> options, const CommandControl& command_control);

  // redis commands:

  RequestAppend Append(std::string key, std::string value,
                       const CommandControl& command_control) override;

  RequestDbsize Dbsize(size_t shard,
                       const CommandControl& command_control) override;

  RequestDel Del(std::string key,
                 const CommandControl& command_control) override;

  RequestDel Del(std::vector<std::string> keys,
                 const CommandControl& command_control) override;

  RequestUnlink Unlink(std::string key,
                       const CommandControl& command_control) override;

  RequestUnlink Unlink(std::vector<std::string> keys,
                       const CommandControl& command_control) override;

  RequestEvalCommon EvalCommon(std::string script,
                               std::vector<std::string> keys,
                               std::vector<std::string> args,
                               const CommandControl& command_control) override;
  RequestEvalShaCommon EvalShaCommon(
      std::string script_hash, std::vector<std::string> keys,
      std::vector<std::string> args,
      const CommandControl& command_control) override;
  RequestScriptLoad ScriptLoad(std::string script, size_t shard,
                               const CommandControl& command_control) override;

  RequestExists Exists(std::string key,
                       const CommandControl& command_control) override;

  RequestExists Exists(std::vector<std::string> keys,
                       const CommandControl& command_control) override;

  RequestExpire Expire(std::string key, std::chrono::seconds ttl,
                       const CommandControl& command_control) override;

  RequestGeoadd Geoadd(std::string key, GeoaddArg point_member,
                       const CommandControl& command_control) override;

  RequestGeoadd Geoadd(std::string key, std::vector<GeoaddArg> point_members,
                       const CommandControl& command_control) override;

  RequestGeoradius Georadius(std::string key, Longitude lon, Latitude lat,
                             double radius,
                             const GeoradiusOptions& georadius_options,
                             const CommandControl& command_control) override;

  RequestGeosearch Geosearch(std::string key, std::string member, double radius,
                             const GeosearchOptions& geosearch_options,
                             const CommandControl& command_control) override;

  RequestGeosearch Geosearch(std::string key, std::string member,
                             BoxWidth width, BoxHeight height,
                             const GeosearchOptions& geosearch_options,
                             const CommandControl& command_control) override;

  RequestGeosearch Geosearch(std::string key, Longitude lon, Latitude lat,
                             double radius,
                             const GeosearchOptions& geosearch_options,
                             const CommandControl& command_control) override;

  RequestGeosearch Geosearch(std::string key, Longitude lon, Latitude lat,
                             BoxWidth width, BoxHeight height,
                             const GeosearchOptions& geosearch_options,
                             const CommandControl& command_control) override;

  RequestGet Get(std::string key,
                 const CommandControl& command_control) override;

  RequestGetset Getset(std::string key, std::string value,
                       const CommandControl& command_control) override;

  RequestHdel Hdel(std::string key, std::string field,
                   const CommandControl& command_control) override;

  RequestHdel Hdel(std::string key, std::vector<std::string> fields,
                   const CommandControl& command_control) override;

  RequestHexists Hexists(std::string key, std::string field,
                         const CommandControl& command_control) override;

  RequestHget Hget(std::string key, std::string field,
                   const CommandControl& command_control) override;

  RequestHgetall Hgetall(std::string key,
                         const CommandControl& command_control) override;

  RequestHincrby Hincrby(std::string key, std::string field, int64_t increment,
                         const CommandControl& command_control) override;

  RequestHincrbyfloat Hincrbyfloat(
      std::string key, std::string field, double increment,
      const CommandControl& command_control) override;

  RequestHkeys Hkeys(std::string key,
                     const CommandControl& command_control) override;

  RequestHlen Hlen(std::string key,
                   const CommandControl& command_control) override;

  RequestHmget Hmget(std::string key, std::vector<std::string> fields,
                     const CommandControl& command_control) override;

  RequestHmset Hmset(
      std::string key,
      std::vector<std::pair<std::string, std::string>> field_values,
      const CommandControl& command_control) override;

  ScanRequest<ScanTag::kHscan> Hscan(
      std::string key, HscanOptions options,
      const CommandControl& command_control) override;

  RequestHset Hset(std::string key, std::string field, std::string value,
                   const CommandControl& command_control) override;

  RequestHsetnx Hsetnx(std::string key, std::string field, std::string value,
                       const CommandControl& command_control) override;

  RequestHvals Hvals(std::string key,
                     const CommandControl& command_control) override;

  RequestIncr Incr(std::string key,
                   const CommandControl& command_control) override;

  RequestKeys Keys(std::string keys_pattern, size_t shard,
                   const CommandControl& command_control) override;

  RequestLindex Lindex(std::string key, int64_t index,
                       const CommandControl& command_control) override;

  RequestLlen Llen(std::string key,
                   const CommandControl& command_control) override;

  RequestLpop Lpop(std::string key,
                   const CommandControl& command_control) override;

  RequestLpush Lpush(std::string key, std::string value,
                     const CommandControl& command_control) override;

  RequestLpush Lpush(std::string key, std::vector<std::string> values,
                     const CommandControl& command_control) override;

  RequestLpushx Lpushx(std::string key, std::string element,
                       const CommandControl& command_control) override;

  RequestLrange Lrange(std::string key, int64_t start, int64_t stop,
                       const CommandControl& command_control) override;

  RequestLrem Lrem(std::string key, int64_t count, std::string element,
                   const CommandControl& command_control) override;

  RequestLtrim Ltrim(std::string key, int64_t start, int64_t stop,
                     const CommandControl& command_control) override;

  RequestMget Mget(std::vector<std::string> keys,
                   const CommandControl& command_control) override;

  RequestMset Mset(std::vector<std::pair<std::string, std::string>> key_values,
                   const CommandControl& command_control) override;

  TransactionPtr Multi() override;

  TransactionPtr Multi(Transaction::CheckShards check_shards) override;

  RequestPersist Persist(std::string key,
                         const CommandControl& command_control) override;

  RequestPexpire Pexpire(std::string key, std::chrono::milliseconds ttl,
                         const CommandControl& command_control) override;

  RequestPing Ping(size_t shard,
                   const CommandControl& command_control) override;

  RequestPingMessage Ping(size_t shard, std::string message,
                          const CommandControl& command_control) override;

  void Publish(std::string channel, std::string message,
               const CommandControl& command_control, PubShard policy) override;

  RequestRename Rename(std::string key, std::string new_key,
                       const CommandControl& command_control) override;

  RequestRpop Rpop(std::string key,
                   const CommandControl& command_control) override;

  RequestRpush Rpush(std::string key, std::string value,
                     const CommandControl& command_control) override;

  RequestRpush Rpush(std::string key, std::vector<std::string> values,
                     const CommandControl& command_control) override;

  RequestRpushx Rpushx(std::string key, std::string element,
                       const CommandControl& command_control) override;

  RequestSadd Sadd(std::string key, std::string member,
                   const CommandControl& command_control) override;

  RequestSadd Sadd(std::string key, std::vector<std::string> members,
                   const CommandControl& command_control) override;

  ScanRequest<ScanTag::kScan> Scan(
      size_t shard, ScanOptions options,
      const CommandControl& command_control) override;

  template <ScanTag scan_tag>
  ScanRequest<scan_tag> ScanTmpl(std::string key,
                                 ScanOptionsTmpl<scan_tag> options,
                                 const CommandControl& command_control);

  RequestScard Scard(std::string key,
                     const CommandControl& command_control) override;

  RequestSet Set(std::string key, std::string value,
                 const CommandControl& command_control) override;

  RequestSet Set(std::string key, std::string value,
                 std::chrono::milliseconds ttl,
                 const CommandControl& command_control) override;

  RequestSetIfExist SetIfExist(std::string key, std::string value,
                               const CommandControl& command_control) override;

  RequestSetIfExist SetIfExist(std::string key, std::string value,
                               std::chrono::milliseconds ttl,
                               const CommandControl& command_control) override;

  RequestSetIfNotExist SetIfNotExist(
      std::string key, std::string value,
      const CommandControl& command_control) override;

  RequestSetIfNotExist SetIfNotExist(
      std::string key, std::string value, std::chrono::milliseconds ttl,
      const CommandControl& command_control) override;

  RequestSetex Setex(std::string key, std::chrono::seconds seconds,
                     std::string value,
                     const CommandControl& command_control) override;

  RequestSismember Sismember(std::string key, std::string member,
                             const CommandControl& command_control) override;

  RequestSmembers Smembers(std::string key,
                           const CommandControl& command_control) override;

  RequestSrandmember Srandmember(
      std::string key, const CommandControl& command_control) override;

  RequestSrandmembers Srandmembers(
      std::string key, int64_t count,
      const CommandControl& command_control) override;

  RequestSrem Srem(std::string key, std::string member,
                   const CommandControl& command_control) override;

  RequestSrem Srem(std::string key, std::vector<std::string> members,
                   const CommandControl& command_control) override;

  ScanRequest<ScanTag::kSscan> Sscan(
      std::string key, SscanOptions options,
      const CommandControl& command_control) override;

  RequestStrlen Strlen(std::string key,
                       const CommandControl& command_control) override;

  RequestTime Time(size_t shard,
                   const CommandControl& command_control) override;

  RequestTtl Ttl(std::string key,
                 const CommandControl& command_control) override;

  RequestType Type(std::string key,
                   const CommandControl& command_control) override;

  RequestZadd Zadd(std::string key, double score, std::string member,
                   const CommandControl& command_control) override;

  RequestZadd Zadd(std::string key, double score, std::string member,
                   const ZaddOptions& options,
                   const CommandControl& command_control) override;

  RequestZadd Zadd(std::string key,
                   std::vector<std::pair<double, std::string>> scored_members,
                   const CommandControl& command_control) override;

  RequestZadd Zadd(std::string key,
                   std::vector<std::pair<double, std::string>> scored_members,
                   const ZaddOptions& options,
                   const CommandControl& command_control) override;

  RequestZaddIncr ZaddIncr(std::string key, double score, std::string member,
                           const CommandControl& command_control) override;

  RequestZaddIncrExisting ZaddIncrExisting(
      std::string key, double score, std::string member,
      const CommandControl& command_control) override;

  RequestZcard Zcard(std::string key,
                     const CommandControl& command_control) override;

  RequestZrange Zrange(std::string key, int64_t start, int64_t stop,
                       const CommandControl& command_control) override;

  RequestZrangeWithScores ZrangeWithScores(
      std::string key, int64_t start, int64_t stop,
      const CommandControl& command_control) override;

  RequestZrangebyscore Zrangebyscore(
      std::string key, double min, double max,
      const CommandControl& command_control) override;

  RequestZrangebyscore Zrangebyscore(
      std::string key, std::string min, std::string max,
      const CommandControl& command_control) override;

  RequestZrangebyscore Zrangebyscore(
      std::string key, double min, double max,
      const RangeOptions& range_options,
      const CommandControl& command_control) override;

  RequestZrangebyscore Zrangebyscore(
      std::string key, std::string min, std::string max,
      const RangeOptions& range_options,
      const CommandControl& command_control) override;

  RequestZrangebyscoreWithScores ZrangebyscoreWithScores(
      std::string key, double min, double max,
      const CommandControl& command_control) override;

  RequestZrangebyscoreWithScores ZrangebyscoreWithScores(
      std::string key, std::string min, std::string max,
      const CommandControl& command_control) override;

  RequestZrangebyscoreWithScores ZrangebyscoreWithScores(
      std::string key, double min, double max,
      const RangeOptions& range_options,
      const CommandControl& command_control) override;

  RequestZrangebyscoreWithScores ZrangebyscoreWithScores(
      std::string key, std::string min, std::string max,
      const RangeOptions& range_options,
      const CommandControl& command_control) override;

  RequestZrem Zrem(std::string key, std::string member,
                   const CommandControl& command_control) override;

  RequestZrem Zrem(std::string key, std::vector<std::string> members,
                   const CommandControl& command_control) override;

  RequestZremrangebyrank Zremrangebyrank(
      std::string key, int64_t start, int64_t stop,
      const CommandControl& command_control) override;

  RequestZremrangebyscore Zremrangebyscore(
      std::string key, double min, double max,
      const CommandControl& command_control) override;

  RequestZremrangebyscore Zremrangebyscore(
      std::string key, std::string min, std::string max,
      const CommandControl& command_control) override;

  ScanRequest<ScanTag::kZscan> Zscan(
      std::string key, ZscanOptions options,
      const CommandControl& command_control) override;

  RequestZscore Zscore(std::string key, std::string member,
                       const CommandControl& command_control) override;

  // end of redis commands

  friend class TransactionImpl;

 private:
  using CmdArgs = USERVER_NAMESPACE::redis::CmdArgs;

  USERVER_NAMESPACE::redis::Request MakeRequest(
      CmdArgs&& args, size_t shard, bool master,
      const CommandControl& command_control, size_t replies_to_skip = 0);

  template <typename T, typename Func>
  auto MakeRequestChunks(size_t max_chunk_size, std::vector<T>&& args,
                         Func&& func) {
    std::vector<USERVER_NAMESPACE::redis::Request> requests;

    auto chunk_size = static_cast<std::ptrdiff_t>(max_chunk_size);
    // NOLINTNEXTLINE(readability-qualified-auto)
    for (auto it = args.begin(); it < args.end(); it += chunk_size) {
      chunk_size = std::min(chunk_size, args.end() - it);
      std::vector<T> args_chunk;
      args_chunk.reserve(chunk_size);
      std::move(it, it + chunk_size, std::back_inserter(args_chunk));
      requests.push_back(func(std::move(args_chunk)));
    }

    return requests;
  }

  CommandControl GetCommandControl(const CommandControl& cc) const;

  size_t GetPublishShard(PubShard policy);

  size_t ShardByKey(const std::string& key, const CommandControl& cc) const;

  void CheckShard(size_t shard, const CommandControl& cc) const;

  std::shared_ptr<USERVER_NAMESPACE::redis::Sentinel> redis_client_;
  std::atomic<int> publish_shard_{0};
  const std::optional<size_t> force_shard_idx_;
};

}  // namespace storages::redis

USERVER_NAMESPACE_END
