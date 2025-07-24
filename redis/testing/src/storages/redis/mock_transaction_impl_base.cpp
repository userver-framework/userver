#include <userver/storages/redis/mock_transaction_impl_base.hpp>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

using utils::AbortWithStacktrace;

// redis commands:

RequestAppend MockTransactionImplBase::Append(std::string /*key*/, std::string /*value*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestBitop
MockTransactionImplBase::Bitop(BitOperation /*op*/, std::string /*dest_key*/, std::vector<std::string> /*src_keys*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestDbsize MockTransactionImplBase::Dbsize(size_t /*shard*/) { AbortWithStacktrace("Redis method not mocked"); }

RequestDecr MockTransactionImplBase::Decr(std::string /*key*/) { AbortWithStacktrace("Redis method not mocked"); }

RequestDel MockTransactionImplBase::Del(std::string /*key*/) { AbortWithStacktrace("Redis method not mocked"); }

RequestDel MockTransactionImplBase::Del(std::vector<std::string> /*keys*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestUnlink MockTransactionImplBase::Unlink(std::string /*key*/) { AbortWithStacktrace("Redis method not mocked"); }

RequestUnlink MockTransactionImplBase::Unlink(std::vector<std::string> /*keys*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestExists MockTransactionImplBase::Exists(std::string /*key*/) { AbortWithStacktrace("Redis method not mocked"); }

RequestExists MockTransactionImplBase::Exists(std::vector<std::string> /*keys*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestExpire MockTransactionImplBase::Expire(std::string /*key*/, std::chrono::seconds /*ttl*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestGeoadd MockTransactionImplBase::Geoadd(std::string /*key*/, GeoaddArg /*point_member*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestGeoadd MockTransactionImplBase::Geoadd(std::string /*key*/, std::vector<GeoaddArg> /*point_members*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestGeoradius MockTransactionImplBase::Georadius(
    std::string /*key*/,
    Longitude /*lon*/,
    Latitude /*lat*/,
    double /*radius*/,
    const GeoradiusOptions& /*georadius_options*/
) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestGeosearch MockTransactionImplBase::Geosearch(
    std::string /*key*/,
    std::string /*member*/,
    double /*radius*/,
    const GeosearchOptions& /*geosearch_options*/
) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestGeosearch MockTransactionImplBase::Geosearch(
    std::string /*key*/,
    std::string /*member*/,
    BoxWidth /*width*/,
    BoxHeight /*height*/,
    const GeosearchOptions& /*geosearch_options*/
) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestGeosearch MockTransactionImplBase::Geosearch(
    std::string /*key*/,
    Longitude /*lon*/,
    Latitude /*lat*/,
    double /*radius*/,
    const GeosearchOptions& /*geosearch_options*/
) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestGeosearch MockTransactionImplBase::Geosearch(
    std::string /*key*/,
    Longitude /*lon*/,
    Latitude /*lat*/,
    BoxWidth /*width*/,
    BoxHeight /*height*/,
    const GeosearchOptions& /*geosearch_options*/
) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestGet MockTransactionImplBase::Get(std::string /*key*/) { AbortWithStacktrace("Redis method not mocked"); }

RequestGetset MockTransactionImplBase::Getset(std::string /*key*/, std::string /*value*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestHdel MockTransactionImplBase::Hdel(std::string /*key*/, std::string /*field*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestHdel MockTransactionImplBase::Hdel(std::string /*key*/, std::vector<std::string> /*fields*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestHexists MockTransactionImplBase::Hexists(std::string /*key*/, std::string /*field*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestHget MockTransactionImplBase::Hget(std::string /*key*/, std::string /*field*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestHgetall MockTransactionImplBase::Hgetall(std::string /*key*/) { AbortWithStacktrace("Redis method not mocked"); }

RequestHincrby MockTransactionImplBase::Hincrby(std::string /*key*/, std::string /*field*/, int64_t /*increment*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestHincrbyfloat
MockTransactionImplBase::Hincrbyfloat(std::string /*key*/, std::string /*field*/, double /*increment*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestHkeys MockTransactionImplBase::Hkeys(std::string /*key*/) { AbortWithStacktrace("Redis method not mocked"); }

RequestHlen MockTransactionImplBase::Hlen(std::string /*key*/) { AbortWithStacktrace("Redis method not mocked"); }

RequestHmget MockTransactionImplBase::Hmget(std::string /*key*/, std::vector<std::string> /*fields*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestHmset
MockTransactionImplBase::Hmset(std::string /*key*/, std::vector<std::pair<std::string, std::string>> /*field_values*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestHset MockTransactionImplBase::Hset(std::string /*key*/, std::string /*field*/, std::string /*value*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestHsetnx MockTransactionImplBase::Hsetnx(std::string /*key*/, std::string /*field*/, std::string /*value*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestHvals MockTransactionImplBase::Hvals(std::string /*key*/) { AbortWithStacktrace("Redis method not mocked"); }

RequestIncr MockTransactionImplBase::Incr(std::string /*key*/) { AbortWithStacktrace("Redis method not mocked"); }

RequestKeys MockTransactionImplBase::Keys(std::string /*keys_pattern*/, size_t /*shard*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestLindex MockTransactionImplBase::Lindex(std::string /*key*/, int64_t /*index*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestLlen MockTransactionImplBase::Llen(std::string /*key*/) { AbortWithStacktrace("Redis method not mocked"); }

RequestLpop MockTransactionImplBase::Lpop(std::string /*key*/) { AbortWithStacktrace("Redis method not mocked"); }

RequestLpush MockTransactionImplBase::Lpush(std::string /*key*/, std::string /*value*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestLpush MockTransactionImplBase::Lpush(std::string /*key*/, std::vector<std::string> /*values*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestLpushx MockTransactionImplBase::Lpushx(std::string /*key*/, std::string /*element*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestLrange MockTransactionImplBase::Lrange(std::string /*key*/, int64_t /*start*/, int64_t /*stop*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestLrem MockTransactionImplBase::Lrem(std::string /*key*/, int64_t /*count*/, std::string /*element*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestLtrim MockTransactionImplBase::Ltrim(std::string /*key*/, int64_t /*start*/, int64_t /*stop*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestMget MockTransactionImplBase::Mget(std::vector<std::string> /*keys*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestMset MockTransactionImplBase::Mset(std::vector<std::pair<std::string, std::string>> /*key_values*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestPersist MockTransactionImplBase::Persist(std::string /*key*/) { AbortWithStacktrace("Redis method not mocked"); }

RequestPexpire MockTransactionImplBase::Pexpire(std::string /*key*/, std::chrono::milliseconds /*ttl*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestPing MockTransactionImplBase::Ping(size_t /*shard*/) { AbortWithStacktrace("Redis method not mocked"); }

RequestPingMessage MockTransactionImplBase::PingMessage(size_t /*shard*/, std::string /*message*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestRename MockTransactionImplBase::Rename(std::string /*key*/, std::string /*new_key*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestRpop MockTransactionImplBase::Rpop(std::string /*key*/) { AbortWithStacktrace("Redis method not mocked"); }

RequestRpush MockTransactionImplBase::Rpush(std::string /*key*/, std::string /*value*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestRpush MockTransactionImplBase::Rpush(std::string /*key*/, std::vector<std::string> /*values*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestRpushx MockTransactionImplBase::Rpushx(std::string /*key*/, std::string /*element*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestSadd MockTransactionImplBase::Sadd(std::string /*key*/, std::string /*member*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestSadd MockTransactionImplBase::Sadd(std::string /*key*/, std::vector<std::string> /*members*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestScard MockTransactionImplBase::Scard(std::string /*key*/) { AbortWithStacktrace("Redis method not mocked"); }

RequestSet MockTransactionImplBase::Set(std::string /*key*/, std::string /*value*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestSet MockTransactionImplBase::Set(std::string /*key*/, std::string /*value*/, std::chrono::milliseconds /*ttl*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestSetIfExist MockTransactionImplBase::SetIfExist(std::string /*key*/, std::string /*value*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestSetIfExist
MockTransactionImplBase::SetIfExist(std::string /*key*/, std::string /*value*/, std::chrono::milliseconds /*ttl*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestSetIfNotExist MockTransactionImplBase::SetIfNotExist(std::string /*key*/, std::string /*value*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestSetIfNotExist
MockTransactionImplBase::SetIfNotExist(std::string /*key*/, std::string /*value*/, std::chrono::milliseconds /*ttl*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestSetIfNotExistOrGet MockTransactionImplBase::SetIfNotExistOrGet(std::string /*key*/, std::string /*value*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestSetIfNotExistOrGet MockTransactionImplBase::
    SetIfNotExistOrGet(std::string /*key*/, std::string /*value*/, std::chrono::milliseconds /*ttl*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestSetex
MockTransactionImplBase::Setex(std::string /*key*/, std::chrono::seconds /*seconds*/, std::string /*value*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestSismember MockTransactionImplBase::Sismember(std::string /*key*/, std::string /*member*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestSmembers MockTransactionImplBase::Smembers(std::string /*key*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestSrandmember MockTransactionImplBase::Srandmember(std::string /*key*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestSrandmembers MockTransactionImplBase::Srandmembers(std::string /*key*/, int64_t /*count*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestSrem MockTransactionImplBase::Srem(std::string /*key*/, std::string /*member*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestSrem MockTransactionImplBase::Srem(std::string /*key*/, std::vector<std::string> /*members*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestStrlen MockTransactionImplBase::Strlen(std::string /*key*/) { AbortWithStacktrace("Redis method not mocked"); }

RequestTime MockTransactionImplBase::Time(size_t /*shard*/) { AbortWithStacktrace("Redis method not mocked"); }

RequestTtl MockTransactionImplBase::Ttl(std::string /*key*/) { AbortWithStacktrace("Redis method not mocked"); }

RequestType MockTransactionImplBase::Type(std::string /*key*/) { AbortWithStacktrace("Redis method not mocked"); }

RequestZadd MockTransactionImplBase::Zadd(std::string /*key*/, double /*score*/, std::string /*member*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestZadd MockTransactionImplBase::
    Zadd(std::string /*key*/, double /*score*/, std::string /*member*/, const ZaddOptions& /*options*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestZadd
MockTransactionImplBase::Zadd(std::string /*key*/, std::vector<std::pair<double, std::string>> /*scored_members*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestZadd MockTransactionImplBase::Zadd(
    std::string /*key*/,
    std::vector<std::pair<double, std::string>> /*scored_members*/,
    const ZaddOptions& /*options*/
) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestZaddIncr MockTransactionImplBase::ZaddIncr(std::string /*key*/, double /*score*/, std::string /*member*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestZaddIncrExisting
MockTransactionImplBase::ZaddIncrExisting(std::string /*key*/, double /*score*/, std::string /*member*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestZcard MockTransactionImplBase::Zcard(std::string /*key*/) { AbortWithStacktrace("Redis method not mocked"); }

RequestZcount MockTransactionImplBase::Zcount(std::string /*key*/, double /*min*/, double /*max*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestZrange MockTransactionImplBase::Zrange(std::string /*key*/, int64_t /*start*/, int64_t /*stop*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestZrangeWithScores
MockTransactionImplBase::ZrangeWithScores(std::string /*key*/, int64_t /*start*/, int64_t /*stop*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestZrangebyscore MockTransactionImplBase::Zrangebyscore(std::string /*key*/, double /*min*/, double /*max*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestZrangebyscore
MockTransactionImplBase::Zrangebyscore(std::string /*key*/, std::string /*min*/, std::string /*max*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestZrangebyscore MockTransactionImplBase::
    Zrangebyscore(std::string /*key*/, double /*min*/, double /*max*/, const RangeOptions& /*range_options*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestZrangebyscore MockTransactionImplBase::Zrangebyscore(
    std::string /*key*/,
    std::string /*min*/,
    std::string /*max*/,
    const RangeOptions& /*range_options*/
) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestZrangebyscoreWithScores
MockTransactionImplBase::ZrangebyscoreWithScores(std::string /*key*/, double /*min*/, double /*max*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestZrangebyscoreWithScores
MockTransactionImplBase::ZrangebyscoreWithScores(std::string /*key*/, std::string /*min*/, std::string /*max*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestZrangebyscoreWithScores MockTransactionImplBase::ZrangebyscoreWithScores(
    std::string /*key*/,
    double /*min*/,
    double /*max*/,
    const RangeOptions& /*range_options*/
) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestZrangebyscoreWithScores MockTransactionImplBase::ZrangebyscoreWithScores(
    std::string /*key*/,
    std::string /*min*/,
    std::string /*max*/,
    const RangeOptions& /*range_options*/
) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestZrem MockTransactionImplBase::Zrem(std::string /*key*/, std::string /*member*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestZrem MockTransactionImplBase::Zrem(std::string /*key*/, std::vector<std::string> /*members*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestZremrangebyrank
MockTransactionImplBase::Zremrangebyrank(std::string /*key*/, int64_t /*start*/, int64_t /*stop*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestZremrangebyscore MockTransactionImplBase::Zremrangebyscore(std::string /*key*/, double /*min*/, double /*max*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestZremrangebyscore
MockTransactionImplBase::Zremrangebyscore(std::string /*key*/, std::string /*min*/, std::string /*max*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestZscore MockTransactionImplBase::Zscore(std::string /*key*/, std::string /*member*/) {
    AbortWithStacktrace("Redis method not mocked");
}

// end of redis commands

}  // namespace storages::redis

USERVER_NAMESPACE_END
