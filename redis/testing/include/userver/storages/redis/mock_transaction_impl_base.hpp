#pragma once

#include <userver/utest/utest.hpp>

#include <userver/storages/redis/transaction.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

class MockTransactionImplBase {
 public:
  virtual ~MockTransactionImplBase() = default;

  // redis commands:

  virtual RequestAppend Append(std::string key, std::string value);

  virtual RequestDbsize Dbsize(size_t shard);

  virtual RequestDel Del(std::string key);

  virtual RequestDel Del(std::vector<std::string> keys);

  virtual RequestUnlink Unlink(std::string key);

  virtual RequestUnlink Unlink(std::vector<std::string> keys);

  virtual RequestExists Exists(std::string key);

  virtual RequestExists Exists(std::vector<std::string> keys);

  virtual RequestExpire Expire(std::string key, std::chrono::seconds ttl);

  virtual RequestGeoadd Geoadd(std::string key, GeoaddArg point_member);

  virtual RequestGeoadd Geoadd(std::string key,
                               std::vector<GeoaddArg> point_members);

  virtual RequestGeoradius Georadius(std::string key, Longitude lon,
                                     Latitude lat, double radius,
                                     const GeoradiusOptions& georadius_options);

  virtual RequestGeosearch Geosearch(std::string key, std::string member,
                                     double radius,
                                     const GeosearchOptions& geosearch_options);

  virtual RequestGeosearch Geosearch(std::string key, std::string member,
                                     BoxWidth width, BoxHeight height,
                                     const GeosearchOptions& geosearch_options);

  virtual RequestGeosearch Geosearch(std::string key, Longitude lon,
                                     Latitude lat, double radius,
                                     const GeosearchOptions& geosearch_options);

  virtual RequestGeosearch Geosearch(std::string key, Longitude lon,
                                     Latitude lat, BoxWidth width,
                                     BoxHeight height,
                                     const GeosearchOptions& geosearch_options);

  virtual RequestGet Get(std::string key);

  virtual RequestGetset Getset(std::string key, std::string value);

  virtual RequestHdel Hdel(std::string key, std::string field);

  virtual RequestHdel Hdel(std::string key, std::vector<std::string> fields);

  virtual RequestHexists Hexists(std::string key, std::string field);

  virtual RequestHget Hget(std::string key, std::string field);

  virtual RequestHgetall Hgetall(std::string key);

  virtual RequestHincrby Hincrby(std::string key, std::string field,
                                 int64_t increment);

  virtual RequestHincrbyfloat Hincrbyfloat(std::string key, std::string field,
                                           double increment);

  virtual RequestHkeys Hkeys(std::string key);

  virtual RequestHlen Hlen(std::string key);

  virtual RequestHmget Hmget(std::string key, std::vector<std::string> fields);

  virtual RequestHmset Hmset(
      std::string key,
      std::vector<std::pair<std::string, std::string>> field_values);

  virtual RequestHset Hset(std::string key, std::string field,
                           std::string value);

  virtual RequestHsetnx Hsetnx(std::string key, std::string field,
                               std::string value);

  virtual RequestHvals Hvals(std::string key);

  virtual RequestIncr Incr(std::string key);

  virtual RequestKeys Keys(std::string keys_pattern, size_t shard);

  virtual RequestLindex Lindex(std::string key, int64_t index);

  virtual RequestLlen Llen(std::string key);

  virtual RequestLpop Lpop(std::string key);

  virtual RequestLpush Lpush(std::string key, std::string value);

  virtual RequestLpush Lpush(std::string key, std::vector<std::string> values);

  virtual RequestLpushx Lpushx(std::string key, std::string element);

  virtual RequestLrange Lrange(std::string key, int64_t start, int64_t stop);

  virtual RequestLrem Lrem(std::string key, int64_t count, std::string element);

  virtual RequestLtrim Ltrim(std::string key, int64_t start, int64_t stop);

  virtual RequestMget Mget(std::vector<std::string> keys);

  virtual RequestMset Mset(
      std::vector<std::pair<std::string, std::string>> key_values);

  virtual RequestPersist Persist(std::string key);

  virtual RequestPexpire Pexpire(std::string key,
                                 std::chrono::milliseconds ttl);

  virtual RequestPing Ping(size_t shard);

  virtual RequestPingMessage PingMessage(size_t shard, std::string message);

  virtual RequestRename Rename(std::string key, std::string new_key);

  virtual RequestRpop Rpop(std::string key);

  virtual RequestRpush Rpush(std::string key, std::string value);

  virtual RequestRpush Rpush(std::string key, std::vector<std::string> values);

  virtual RequestRpushx Rpushx(std::string key, std::string element);

  virtual RequestSadd Sadd(std::string key, std::string member);

  virtual RequestSadd Sadd(std::string key, std::vector<std::string> members);

  virtual RequestScard Scard(std::string key);

  virtual RequestSet Set(std::string key, std::string value);

  virtual RequestSet Set(std::string key, std::string value,
                         std::chrono::milliseconds ttl);

  virtual RequestSetIfExist SetIfExist(std::string key, std::string value);

  virtual RequestSetIfExist SetIfExist(std::string key, std::string value,
                                       std::chrono::milliseconds ttl);

  virtual RequestSetIfNotExist SetIfNotExist(std::string key,
                                             std::string value);

  virtual RequestSetIfNotExist SetIfNotExist(std::string key, std::string value,
                                             std::chrono::milliseconds ttl);

  virtual RequestSetex Setex(std::string key, std::chrono::seconds seconds,
                             std::string value);

  virtual RequestSismember Sismember(std::string key, std::string member);

  virtual RequestSmembers Smembers(std::string key);

  virtual RequestSrandmember Srandmember(std::string key);

  virtual RequestSrandmembers Srandmembers(std::string key, int64_t count);

  virtual RequestSrem Srem(std::string key, std::string member);

  virtual RequestSrem Srem(std::string key, std::vector<std::string> members);

  virtual RequestStrlen Strlen(std::string key);

  virtual RequestTime Time(size_t shard);

  virtual RequestTtl Ttl(std::string key);

  virtual RequestType Type(std::string key);

  virtual RequestZadd Zadd(std::string key, double score, std::string member);

  virtual RequestZadd Zadd(std::string key, double score, std::string member,
                           const ZaddOptions& options);

  virtual RequestZadd Zadd(
      std::string key,
      std::vector<std::pair<double, std::string>> scored_members);

  virtual RequestZadd Zadd(
      std::string key,
      std::vector<std::pair<double, std::string>> scored_members,
      const ZaddOptions& options);

  virtual RequestZaddIncr ZaddIncr(std::string key, double score,
                                   std::string member);

  virtual RequestZaddIncrExisting ZaddIncrExisting(std::string key,
                                                   double score,
                                                   std::string member);

  virtual RequestZcard Zcard(std::string key);

  virtual RequestZrange Zrange(std::string key, int64_t start, int64_t stop);

  virtual RequestZrangeWithScores ZrangeWithScores(std::string key,
                                                   int64_t start, int64_t stop);

  virtual RequestZrangebyscore Zrangebyscore(std::string key, double min,
                                             double max);

  virtual RequestZrangebyscore Zrangebyscore(std::string key, std::string min,
                                             std::string max);

  virtual RequestZrangebyscore Zrangebyscore(std::string key, double min,
                                             double max,
                                             const RangeOptions& range_options);

  virtual RequestZrangebyscore Zrangebyscore(std::string key, std::string min,
                                             std::string max,
                                             const RangeOptions& range_options);

  virtual RequestZrangebyscoreWithScores ZrangebyscoreWithScores(
      std::string key, double min, double max);

  virtual RequestZrangebyscoreWithScores ZrangebyscoreWithScores(
      std::string key, std::string min, std::string max);

  virtual RequestZrangebyscoreWithScores ZrangebyscoreWithScores(
      std::string key, double min, double max,
      const RangeOptions& range_options);

  virtual RequestZrangebyscoreWithScores ZrangebyscoreWithScores(
      std::string key, std::string min, std::string max,
      const RangeOptions& range_options);

  virtual RequestZrem Zrem(std::string key, std::string member);

  virtual RequestZrem Zrem(std::string key, std::vector<std::string> members);

  virtual RequestZremrangebyrank Zremrangebyrank(std::string key, int64_t start,
                                                 int64_t stop);

  virtual RequestZremrangebyscore Zremrangebyscore(std::string key, double min,
                                                   double max);

  virtual RequestZremrangebyscore Zremrangebyscore(std::string key,
                                                   std::string min,
                                                   std::string max);

  virtual RequestZscore Zscore(std::string key, std::string member);

  // end of redis commands
};

}  // namespace storages::redis

USERVER_NAMESPACE_END
