#pragma once

/// @file userver/storages/redis/transaction.hpp
/// @brief @copybrief storages::redis::Transaction

#include <memory>
#include <string>

#include <userver/storages/redis/command_options.hpp>
#include <userver/storages/redis/request.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

/// Atomic sequence of commands (https://redis.io/topics/transactions).
/// Please note that Redis transaction implements isolation, but not
/// all-or-nothing semantics (IOW a subcommand may fail, but the following
/// subcommands will succeed).
/// Methods will add commands to the `Transaction` object.
/// For each added command a future-like object will be returned.
/// You can get the result of each transaction's subcommand by calling `Get()`
/// method for these objects.
/// Commands will be sent to a server after calling `Exec()` that returns
/// `RequestExec` object.
/// You should not call `Get()` method in a future-like subcommand's object
/// before calling `Get()` method on `RequestExec` object.
class Transaction {
 public:
  enum class CheckShards { kNo, kSame };

  virtual ~Transaction() = default;

  /// Finish current atomic sequence of commands and send it to a server.
  /// Returns 'future-like' request object.
  /// The data will not be set for the future-like objects for subcommands if
  /// `Get()` method of the returned object is not called or redis did not
  /// return an array with command responses.
  /// In the last case `Get()` will throw a corresponding exception.
  virtual RequestExec Exec(const CommandControl& command_control) = 0;

  // redis commands:

  virtual RequestAppend Append(std::string key, std::string value) = 0;

  virtual RequestDbsize Dbsize(size_t shard) = 0;

  virtual RequestDel Del(std::string key) = 0;

  virtual RequestDel Del(std::vector<std::string> keys) = 0;

  virtual RequestUnlink Unlink(std::string key) = 0;

  virtual RequestUnlink Unlink(std::vector<std::string> keys) = 0;

  virtual RequestExists Exists(std::string key) = 0;

  virtual RequestExists Exists(std::vector<std::string> keys) = 0;

  virtual RequestExpire Expire(std::string key, std::chrono::seconds ttl) = 0;

  virtual RequestGeoadd Geoadd(std::string key, GeoaddArg point_member) = 0;

  virtual RequestGeoadd Geoadd(std::string key,
                               std::vector<GeoaddArg> point_members) = 0;

  virtual RequestGeoradius Georadius(
      std::string key, Longitude lon, Latitude lat, double radius,
      const GeoradiusOptions& georadius_options) = 0;

  virtual RequestGeosearch Geosearch(
      std::string key, std::string member, double radius,
      const GeosearchOptions& geosearch_options) = 0;

  virtual RequestGeosearch Geosearch(
      std::string key, std::string member, BoxWidth width, BoxHeight height,
      const GeosearchOptions& geosearch_options) = 0;

  virtual RequestGeosearch Geosearch(
      std::string key, Longitude lon, Latitude lat, double radius,
      const GeosearchOptions& geosearch_options) = 0;

  virtual RequestGeosearch Geosearch(
      std::string key, Longitude lon, Latitude lat, BoxWidth width,
      BoxHeight height, const GeosearchOptions& geosearch_options) = 0;

  virtual RequestGet Get(std::string key) = 0;

  virtual RequestGetset Getset(std::string key, std::string value) = 0;

  virtual RequestHdel Hdel(std::string key, std::string field) = 0;

  virtual RequestHdel Hdel(std::string key,
                           std::vector<std::string> fields) = 0;

  virtual RequestHexists Hexists(std::string key, std::string field) = 0;

  virtual RequestHget Hget(std::string key, std::string field) = 0;

  virtual RequestHgetall Hgetall(std::string key) = 0;

  virtual RequestHincrby Hincrby(std::string key, std::string field,
                                 int64_t increment) = 0;

  virtual RequestHincrbyfloat Hincrbyfloat(std::string key, std::string field,
                                           double increment) = 0;

  virtual RequestHkeys Hkeys(std::string key) = 0;

  virtual RequestHlen Hlen(std::string key) = 0;

  virtual RequestHmget Hmget(std::string key,
                             std::vector<std::string> fields) = 0;

  virtual RequestHmset Hmset(
      std::string key,
      std::vector<std::pair<std::string, std::string>> field_values) = 0;

  virtual RequestHset Hset(std::string key, std::string field,
                           std::string value) = 0;

  virtual RequestHsetnx Hsetnx(std::string key, std::string field,
                               std::string value) = 0;

  virtual RequestHvals Hvals(std::string key) = 0;

  virtual RequestIncr Incr(std::string key) = 0;

  virtual RequestKeys Keys(std::string keys_pattern, size_t shard) = 0;

  virtual RequestLindex Lindex(std::string key, int64_t index) = 0;

  virtual RequestLlen Llen(std::string key) = 0;

  virtual RequestLpop Lpop(std::string key) = 0;

  virtual RequestLpush Lpush(std::string key, std::string value) = 0;

  virtual RequestLpush Lpush(std::string key,
                             std::vector<std::string> values) = 0;

  virtual RequestLpushx Lpushx(std::string key, std::string element) = 0;

  virtual RequestLrange Lrange(std::string key, int64_t start,
                               int64_t stop) = 0;

  virtual RequestLrem Lrem(std::string key, int64_t count,
                           std::string element) = 0;

  virtual RequestLtrim Ltrim(std::string key, int64_t start, int64_t stop) = 0;

  virtual RequestMget Mget(std::vector<std::string> keys) = 0;

  virtual RequestMset Mset(
      std::vector<std::pair<std::string, std::string>> key_values) = 0;

  virtual RequestPersist Persist(std::string key) = 0;

  virtual RequestPexpire Pexpire(std::string key,
                                 std::chrono::milliseconds ttl) = 0;

  virtual RequestPing Ping(size_t shard) = 0;

  virtual RequestPingMessage PingMessage(size_t shard, std::string message) = 0;

  virtual RequestRename Rename(std::string key, std::string new_key) = 0;

  virtual RequestRpop Rpop(std::string key) = 0;

  virtual RequestRpush Rpush(std::string key, std::string value) = 0;

  virtual RequestRpush Rpush(std::string key,
                             std::vector<std::string> values) = 0;

  virtual RequestRpushx Rpushx(std::string key, std::string element) = 0;

  virtual RequestSadd Sadd(std::string key, std::string member) = 0;

  virtual RequestSadd Sadd(std::string key,
                           std::vector<std::string> members) = 0;

  virtual RequestScard Scard(std::string key) = 0;

  virtual RequestSet Set(std::string key, std::string value) = 0;

  virtual RequestSet Set(std::string key, std::string value,
                         std::chrono::milliseconds ttl) = 0;

  virtual RequestSetIfExist SetIfExist(std::string key, std::string value) = 0;

  virtual RequestSetIfExist SetIfExist(std::string key, std::string value,
                                       std::chrono::milliseconds ttl) = 0;

  virtual RequestSetIfNotExist SetIfNotExist(std::string key,
                                             std::string value) = 0;

  virtual RequestSetIfNotExist SetIfNotExist(std::string key, std::string value,
                                             std::chrono::milliseconds ttl) = 0;

  virtual RequestSetex Setex(std::string key, std::chrono::seconds seconds,
                             std::string value) = 0;

  virtual RequestSismember Sismember(std::string key, std::string member) = 0;

  virtual RequestSmembers Smembers(std::string key) = 0;

  virtual RequestSrandmember Srandmember(std::string key) = 0;

  virtual RequestSrandmembers Srandmembers(std::string key, int64_t count) = 0;

  virtual RequestSrem Srem(std::string key, std::string member) = 0;

  virtual RequestSrem Srem(std::string key,
                           std::vector<std::string> members) = 0;

  virtual RequestStrlen Strlen(std::string key) = 0;

  virtual RequestTime Time(size_t shard) = 0;

  virtual RequestTtl Ttl(std::string key) = 0;

  virtual RequestType Type(std::string key) = 0;

  virtual RequestZadd Zadd(std::string key, double score,
                           std::string member) = 0;

  virtual RequestZadd Zadd(std::string key, double score, std::string member,
                           const ZaddOptions& options) = 0;

  virtual RequestZadd Zadd(
      std::string key,
      std::vector<std::pair<double, std::string>> scored_members) = 0;

  virtual RequestZadd Zadd(
      std::string key,
      std::vector<std::pair<double, std::string>> scored_members,
      const ZaddOptions& options) = 0;

  virtual RequestZaddIncr ZaddIncr(std::string key, double score,
                                   std::string member) = 0;

  virtual RequestZaddIncrExisting ZaddIncrExisting(std::string key,
                                                   double score,
                                                   std::string member) = 0;

  virtual RequestZcard Zcard(std::string key) = 0;

  virtual RequestZrange Zrange(std::string key, int64_t start,
                               int64_t stop) = 0;

  virtual RequestZrangeWithScores ZrangeWithScores(std::string key,
                                                   int64_t start,
                                                   int64_t stop) = 0;

  virtual RequestZrangebyscore Zrangebyscore(std::string key, double min,
                                             double max) = 0;

  virtual RequestZrangebyscore Zrangebyscore(std::string key, std::string min,
                                             std::string max) = 0;

  virtual RequestZrangebyscore Zrangebyscore(
      std::string key, double min, double max,
      const RangeOptions& range_options) = 0;

  virtual RequestZrangebyscore Zrangebyscore(
      std::string key, std::string min, std::string max,
      const RangeOptions& range_options) = 0;

  virtual RequestZrangebyscoreWithScores ZrangebyscoreWithScores(
      std::string key, double min, double max) = 0;

  virtual RequestZrangebyscoreWithScores ZrangebyscoreWithScores(
      std::string key, std::string min, std::string max) = 0;

  virtual RequestZrangebyscoreWithScores ZrangebyscoreWithScores(
      std::string key, double min, double max,
      const RangeOptions& range_options) = 0;

  virtual RequestZrangebyscoreWithScores ZrangebyscoreWithScores(
      std::string key, std::string min, std::string max,
      const RangeOptions& range_options) = 0;

  virtual RequestZrem Zrem(std::string key, std::string member) = 0;

  virtual RequestZrem Zrem(std::string key,
                           std::vector<std::string> members) = 0;

  virtual RequestZremrangebyrank Zremrangebyrank(std::string key, int64_t start,
                                                 int64_t stop) = 0;

  virtual RequestZremrangebyscore Zremrangebyscore(std::string key, double min,
                                                   double max) = 0;

  virtual RequestZremrangebyscore Zremrangebyscore(std::string key,
                                                   std::string min,
                                                   std::string max) = 0;

  virtual RequestZscore Zscore(std::string key, std::string member) = 0;

  // end of redis commands
};

using TransactionPtr = std::unique_ptr<Transaction>;

class EmptyTransactionException : public USERVER_NAMESPACE::redis::Exception {
 public:
  using USERVER_NAMESPACE::redis::Exception::Exception;
};

class NotStartedTransactionException
    : public USERVER_NAMESPACE::redis::Exception {
 public:
  using USERVER_NAMESPACE::redis::Exception::Exception;
};

}  // namespace storages::redis

USERVER_NAMESPACE_END
