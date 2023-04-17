#pragma once

#include <string>
#include <vector>

#include <userver/storages/redis/impl/base.hpp>

#include <userver/engine/future.hpp>
#include <userver/storages/redis/client.hpp>
#include <userver/storages/redis/transaction.hpp>

#include "request_data_impl.hpp"

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

class ClientImpl;

class TransactionImpl final : public Transaction {
 public:
  explicit TransactionImpl(std::shared_ptr<ClientImpl> client,
                           CheckShards check_shards = CheckShards::kSame);

  RequestExec Exec(const CommandControl& command_control) override;

  class ResultPromise {
   public:
    template <typename Result, typename ReplyType>
    ResultPromise(engine::Promise<ReplyType>&& promise,
                  To<Request<Result, ReplyType>>)
        : impl_(std::make_unique<ResultPromiseImpl<Result, ReplyType>>(
              std::move(promise))) {}
    ResultPromise(ResultPromise&& other) = default;

    void ProcessReply(ReplyData&& reply_data,
                      const std::string& request_description) {
      impl_->ProcessReply(std::move(reply_data), request_description);
    }

   private:
    class ResultPromiseImplBase {
     public:
      virtual ~ResultPromiseImplBase() = default;

      virtual void ProcessReply(ReplyData&& reply_data,
                                const std::string& request_description) = 0;
    };

    template <typename Result, typename ReplyType>
    class ResultPromiseImpl : public ResultPromiseImplBase {
     public:
      ResultPromiseImpl(engine::Promise<ReplyType>&& promise)
          : promise_(std::move(promise)) {}

      void ProcessReply(ReplyData&& reply_data,
                        const std::string& request_description) override {
        try {
          if constexpr (std::is_same<ReplyType, void>::value) {
            Parse(std::move(reply_data), request_description,
                  To<Result, ReplyType>{});
            promise_.set_value();
          } else {
            promise_.set_value(Parse(std::move(reply_data), request_description,
                                     To<Result, ReplyType>{}));
          }
        } catch (const std::exception&) {
          promise_.set_exception(std::current_exception());
        }
      }

     private:
      engine::Promise<ReplyType> promise_;
    };

    std::unique_ptr<ResultPromiseImplBase> impl_;
  };

  // redis commands:

  RequestAppend Append(std::string key, std::string value) override;

  RequestDbsize Dbsize(size_t shard) override;

  RequestDel Del(std::string key) override;

  RequestDel Del(std::vector<std::string> keys) override;

  RequestUnlink Unlink(std::string key) override;

  RequestUnlink Unlink(std::vector<std::string> keys) override;

  RequestExists Exists(std::string key) override;

  RequestExists Exists(std::vector<std::string> keys) override;

  RequestExpire Expire(std::string key, std::chrono::seconds ttl) override;

  RequestGeoadd Geoadd(std::string key, GeoaddArg point_member) override;

  RequestGeoadd Geoadd(std::string key,
                       std::vector<GeoaddArg> point_members) override;

  RequestGeoradius Georadius(
      std::string key, Longitude lon, Latitude lat, double radius,
      const GeoradiusOptions& georadius_options) override;

  RequestGeosearch Geosearch(
      std::string key, std::string member, double radius,
      const GeosearchOptions& geosearch_options) override;

  RequestGeosearch Geosearch(
      std::string key, std::string member, BoxWidth width, BoxHeight height,
      const GeosearchOptions& geosearch_options) override;

  RequestGeosearch Geosearch(
      std::string key, Longitude lon, Latitude lat, double radius,
      const GeosearchOptions& geosearch_options) override;

  RequestGeosearch Geosearch(
      std::string key, Longitude lon, Latitude lat, BoxWidth width,
      BoxHeight height, const GeosearchOptions& geosearch_options) override;

  RequestGet Get(std::string key) override;

  RequestGetset Getset(std::string key, std::string value) override;

  RequestHdel Hdel(std::string key, std::string field) override;

  RequestHdel Hdel(std::string key, std::vector<std::string> fields) override;

  RequestHexists Hexists(std::string key, std::string field) override;

  RequestHget Hget(std::string key, std::string field) override;

  RequestHgetall Hgetall(std::string key) override;

  RequestHincrby Hincrby(std::string key, std::string field,
                         int64_t increment) override;

  RequestHincrbyfloat Hincrbyfloat(std::string key, std::string field,
                                   double increment) override;

  RequestHkeys Hkeys(std::string key) override;

  RequestHlen Hlen(std::string key) override;

  RequestHmget Hmget(std::string key, std::vector<std::string> fields) override;

  RequestHmset Hmset(
      std::string key,
      std::vector<std::pair<std::string, std::string>> field_values) override;

  RequestHset Hset(std::string key, std::string field,
                   std::string value) override;

  RequestHsetnx Hsetnx(std::string key, std::string field,
                       std::string value) override;

  RequestHvals Hvals(std::string key) override;

  RequestIncr Incr(std::string key) override;

  RequestKeys Keys(std::string keys_pattern, size_t shard) override;

  RequestLindex Lindex(std::string key, int64_t index) override;

  RequestLlen Llen(std::string key) override;

  RequestLpop Lpop(std::string key) override;

  RequestLpush Lpush(std::string key, std::string value) override;

  RequestLpush Lpush(std::string key, std::vector<std::string> values) override;

  RequestLpushx Lpushx(std::string key, std::string element) override;

  RequestLrange Lrange(std::string key, int64_t start, int64_t stop) override;

  RequestLrem Lrem(std::string key, int64_t count,
                   std::string element) override;

  RequestLtrim Ltrim(std::string key, int64_t start, int64_t stop) override;

  RequestMget Mget(std::vector<std::string> keys) override;

  RequestMset Mset(
      std::vector<std::pair<std::string, std::string>> key_values) override;

  RequestPersist Persist(std::string key) override;

  RequestPexpire Pexpire(std::string key,
                         std::chrono::milliseconds ttl) override;

  RequestPing Ping(size_t shard) override;

  RequestPingMessage PingMessage(size_t shard, std::string message) override;

  RequestRename Rename(std::string key, std::string new_key) override;

  RequestRpop Rpop(std::string key) override;

  RequestRpush Rpush(std::string key, std::string value) override;

  RequestRpush Rpush(std::string key, std::vector<std::string> values) override;

  RequestRpushx Rpushx(std::string key, std::string element) override;

  RequestSadd Sadd(std::string key, std::string member) override;

  RequestSadd Sadd(std::string key, std::vector<std::string> members) override;

  RequestScard Scard(std::string key) override;

  RequestSet Set(std::string key, std::string value) override;

  RequestSet Set(std::string key, std::string value,
                 std::chrono::milliseconds ttl) override;

  RequestSetIfExist SetIfExist(std::string key, std::string value) override;

  RequestSetIfExist SetIfExist(std::string key, std::string value,
                               std::chrono::milliseconds ttl) override;

  RequestSetIfNotExist SetIfNotExist(std::string key,
                                     std::string value) override;

  RequestSetIfNotExist SetIfNotExist(std::string key, std::string value,
                                     std::chrono::milliseconds ttl) override;

  RequestSetex Setex(std::string key, std::chrono::seconds seconds,
                     std::string value) override;

  RequestSismember Sismember(std::string key, std::string member) override;

  RequestSmembers Smembers(std::string key) override;

  RequestSrandmember Srandmember(std::string key) override;

  RequestSrandmembers Srandmembers(std::string key, int64_t count) override;

  RequestSrem Srem(std::string key, std::string member) override;

  RequestSrem Srem(std::string key, std::vector<std::string> members) override;

  RequestStrlen Strlen(std::string key) override;

  RequestTime Time(size_t shard) override;

  RequestTtl Ttl(std::string key) override;

  RequestType Type(std::string key) override;

  RequestZadd Zadd(std::string key, double score, std::string member) override;

  RequestZadd Zadd(std::string key, double score, std::string member,
                   const ZaddOptions& options) override;

  RequestZadd Zadd(
      std::string key,
      std::vector<std::pair<double, std::string>> scored_members) override;

  RequestZadd Zadd(std::string key,
                   std::vector<std::pair<double, std::string>> scored_members,
                   const ZaddOptions& options) override;

  RequestZaddIncr ZaddIncr(std::string key, double score,
                           std::string member) override;

  RequestZaddIncrExisting ZaddIncrExisting(std::string key, double score,
                                           std::string member) override;

  RequestZcard Zcard(std::string key) override;

  RequestZrange Zrange(std::string key, int64_t start, int64_t stop) override;

  RequestZrangeWithScores ZrangeWithScores(std::string key, int64_t start,
                                           int64_t stop) override;

  RequestZrangebyscore Zrangebyscore(std::string key, double min,
                                     double max) override;

  RequestZrangebyscore Zrangebyscore(std::string key, std::string min,
                                     std::string max) override;

  RequestZrangebyscore Zrangebyscore(
      std::string key, double min, double max,
      const RangeOptions& range_options) override;

  RequestZrangebyscore Zrangebyscore(
      std::string key, std::string min, std::string max,
      const RangeOptions& range_options) override;

  RequestZrangebyscoreWithScores ZrangebyscoreWithScores(std::string key,
                                                         double min,
                                                         double max) override;

  RequestZrangebyscoreWithScores ZrangebyscoreWithScores(
      std::string key, std::string min, std::string max) override;

  RequestZrangebyscoreWithScores ZrangebyscoreWithScores(
      std::string key, double min, double max,
      const RangeOptions& range_options) override;

  RequestZrangebyscoreWithScores ZrangebyscoreWithScores(
      std::string key, std::string min, std::string max,
      const RangeOptions& range_options) override;

  RequestZrem Zrem(std::string key, std::string member) override;

  RequestZrem Zrem(std::string key, std::vector<std::string> members) override;

  RequestZremrangebyrank Zremrangebyrank(std::string key, int64_t start,
                                         int64_t stop) override;

  RequestZremrangebyscore Zremrangebyscore(std::string key, double min,
                                           double max) override;

  RequestZremrangebyscore Zremrangebyscore(std::string key, std::string min,
                                           std::string max) override;

  RequestZscore Zscore(std::string key, std::string member) override;

  // end of redis commands

 private:
  void UpdateShard(const std::string& key);
  void UpdateShard(const std::vector<std::string>& keys);
  void UpdateShard(
      const std::vector<std::pair<std::string, std::string>>& key_values);
  void UpdateShard(size_t shard);

  template <typename Result, typename ReplyType>
  Request<Result, ReplyType> DoAddCmd(To<Request<Result, ReplyType>>);

  template <typename Request, typename... Args>
  Request AddCmd(std::string command, bool master, Args&&... args);

  std::shared_ptr<ClientImpl> client_;
  const CheckShards check_shards_;

  std::optional<size_t> shard_;

  bool master_{};
  USERVER_NAMESPACE::redis::CmdArgs cmd_args_;
  std::vector<ResultPromise> result_promises_;
};

}  // namespace storages::redis

USERVER_NAMESPACE_END
