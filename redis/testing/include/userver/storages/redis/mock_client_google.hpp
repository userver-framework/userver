#pragma once

#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <userver/utest/utest.hpp>

#include <userver/storages/redis/mock_client_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

class GMockClient : public MockClientBase {
 public:
  MOCK_METHOD(size_t, ShardsCount, (), (const, override));

  MOCK_METHOD(size_t, ShardByKey, (const std::string& key), (const, override));

  MOCK_METHOD(const std::string&, GetAnyKeyForShard, (size_t shard_idx),
              (const, override));

  MOCK_METHOD(std::shared_ptr<Client>, GetClientForShard, (size_t shard_idx),
              (override));

  MOCK_METHOD(void, WaitConnectedOnce,
              (USERVER_NAMESPACE::redis::RedisWaitConnected wait_connected),
              (override));

  MOCK_METHOD(RequestAppend, Append,
              (std::string key, std::string value,
               const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestDbsize, Dbsize,
              (size_t shard, const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestDel, Del,
              (std::string key, const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestDel, Del,
              (std::vector<std::string> keys,
               const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestUnlink, Unlink,
              (std::string key, const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestUnlink, Unlink,
              (std::vector<std::string> keys,
               const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestExists, Exists,
              (std::string key, const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestExists, Exists,
              (std::vector<std::string> keys,
               const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestExpire, Expire,
              (std::string key, std::chrono::seconds ttl,
               const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestGet, Get,
              (std::string key, const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestGetset, Getset,
              (std::string key, std::string value,
               const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestHdel, Hdel,
              (std::string key, std::string field,
               const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestHdel, Hdel,
              (std::string key, std::vector<std::string> fields,
               const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestHexists, Hexists,
              (std::string key, std::string field,
               const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestHget, Hget,
              (std::string key, std::string field,
               const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestHgetall, Hgetall,
              (std::string key, const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestHincrby, Hincrby,
              (std::string key, std::string field, int64_t increment,
               const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestHincrbyfloat, Hincrbyfloat,
              (std::string key, std::string field, double increment,
               const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestHkeys, Hkeys,
              (std::string key, const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestHlen, Hlen,
              (std::string key, const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestHmget, Hmget,
              (std::string key, std::vector<std::string> fields,
               const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestHmset, Hmset,
              (std::string key,
               (std::vector<std::pair<std::string, std::string>>)field_values,
               const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestHscan, Hscan,
              (std::string key, HscanOptions options,
               const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestHset, Hset,
              (std::string key, std::string field, std::string value,
               const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestHsetnx, Hsetnx,
              (std::string key, std::string field, std::string value,
               const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestHvals, Hvals,
              (std::string key, const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestIncr, Incr,
              (std::string key, const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestKeys, Keys,
              (std::string keys_pattern, size_t shard,
               const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestLindex, Lindex,
              (std::string key, int64_t index,
               const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestLlen, Llen,
              (std::string key, const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestLpop, Lpop,
              (std::string key, const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestLpush, Lpush,
              (std::string key, std::string value,
               const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestLpush, Lpush,
              (std::string key, std::vector<std::string> values,
               const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestLrange, Lrange,
              (std::string key, int64_t start, int64_t stop,
               const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestLrem, Lrem,
              (std::string key, int64_t count, std::string element,
               const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestLtrim, Ltrim,
              (std::string key, int64_t start, int64_t stop,
               const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestMget, Mget,
              (std::vector<std::string> keys,
               const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestMset, Mset,
              ((std::vector<std::pair<std::string, std::string>>)key_values,
               const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestPersist, Persist,
              (std::string key, const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestPexpire, Pexpire,
              (std::string key, std::chrono::milliseconds ttl,
               const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestPing, Ping,
              (size_t shard, const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestPingMessage, Ping,
              (size_t shard, std::string message,
               const CommandControl& command_control),
              (override));

  MOCK_METHOD(void, Publish,
              (std::string channel, std::string message,
               const CommandControl& command_control, PubShard policy),
              (override));

  MOCK_METHOD(RequestRename, Rename,
              (std::string key, std::string new_key,
               const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestRpop, Rpop,
              (std::string key, const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestRpush, Rpush,
              (std::string key, std::string value,
               const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestRpush, Rpush,
              (std::string key, std::vector<std::string> values,
               const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestSadd, Sadd,
              (std::string key, std::string member,
               const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestSadd, Sadd,
              (std::string key, std::vector<std::string> members,
               const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestScan, Scan,
              (size_t shard, ScanOptions options,
               const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestScard, Scard,
              (std::string key, const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestSet, Set,
              (std::string key, std::string value,
               const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestSet, Set,
              (std::string key, std::string value,
               std::chrono::milliseconds ttl,
               const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestSetIfExist, SetIfExist,
              (std::string key, std::string value,
               const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestSetIfExist, SetIfExist,
              (std::string key, std::string value,
               std::chrono::milliseconds ttl,
               const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestSetIfNotExist, SetIfNotExist,
              (std::string key, std::string value,
               const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestSetIfNotExist, SetIfNotExist,
              (std::string key, std::string value,
               std::chrono::milliseconds ttl,
               const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestSetex, Setex,
              (std::string key, std::chrono::seconds seconds, std::string value,
               const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestSismember, Sismember,
              (std::string key, std::string member,
               const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestSmembers, Smembers,
              (std::string key, const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestSrandmember, Srandmember,
              (std::string key, const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestSrandmembers, Srandmembers,
              (std::string key, int64_t count,
               const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestSrem, Srem,
              (std::string key, std::string member,
               const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestSrem, Srem,
              (std::string key, std::vector<std::string> members,
               const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestSscan, Sscan,
              (std::string key, SscanOptions options,
               const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestStrlen, Strlen,
              (std::string key, const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestTtl, Ttl,
              (std::string key, const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestType, Type,
              (std::string key, const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestZadd, Zadd,
              (std::string key, double score, std::string member,
               const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestZadd, Zadd,
              (std::string key, double score, std::string member,
               const ZaddOptions& options,
               const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestZaddIncr, ZaddIncr,
              (std::string key, double score, std::string member,
               const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestZaddIncrExisting, ZaddIncrExisting,
              (std::string key, double score, std::string member,
               const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestZcard, Zcard,
              (std::string key, const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestZrangeWithScores, ZrangeWithScores,
              (std::string key, int64_t start, int64_t stop,
               const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestZrangebyscore, Zrangebyscore,
              (std::string key, double min, double max,
               const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestZrangebyscore, Zrangebyscore,
              (std::string key, std::string min, std::string max,
               const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestZrangebyscore, Zrangebyscore,
              (std::string key, double min, double max,
               const RangeOptions& range_options,
               const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestZrangebyscore, Zrangebyscore,
              (std::string key, std::string min, std::string max,
               const RangeOptions& range_options,
               const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestZrangebyscoreWithScores, ZrangebyscoreWithScores,
              (std::string key, double min, double max,
               const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestZrangebyscoreWithScores, ZrangebyscoreWithScores,
              (std::string key, std::string min, std::string max,
               const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestZrangebyscoreWithScores, ZrangebyscoreWithScores,
              (std::string key, double min, double max,
               const RangeOptions& range_options,
               const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestZrangebyscoreWithScores, ZrangebyscoreWithScores,
              (std::string key, std::string min, std::string max,
               const RangeOptions& range_options,
               const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestZrem, Zrem,
              (std::string key, std::string member,
               const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestZrem, Zrem,
              (std::string key, std::vector<std::string> members,
               const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestZscan, Zscan,
              (std::string key, ZscanOptions options,
               const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestZscore, Zscore,
              (std::string key, std::string member,
               const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestEvalCommon, EvalCommon,
              (std::string script, std::vector<std::string> keys,
               std::vector<std::string> args,
               const CommandControl& command_control),
              (override));

  MOCK_METHOD(RequestGeosearch, Geosearch,
              (std::string key, std::string member, double radius,
               const GeosearchOptions& geosearch_options,
               const CommandControl& command_control),
              (override));
  MOCK_METHOD(RequestGeosearch, Geosearch,
              (std::string key, std::string member, BoxWidth width,
               BoxHeight height, const GeosearchOptions& geosearch_options,
               const CommandControl& command_control),
              (override));
  MOCK_METHOD(RequestGeosearch, Geosearch,
              (std::string key, Longitude lon, Latitude lat, double radius,
               const GeosearchOptions& geosearch_options,
               const CommandControl& command_control),
              (override));
  MOCK_METHOD(RequestGeosearch, Geosearch,
              (std::string key, Longitude lon, Latitude lat, BoxWidth width,
               BoxHeight height, const GeosearchOptions& geosearch_options,
               const CommandControl& command_control),
              (override));
};

}  // namespace storages::redis

USERVER_NAMESPACE_END
