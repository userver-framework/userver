#include <userver/storages/redis/mock_pipeline_impl_base.hpp>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

using utils::AbortWithStacktrace;

// redis commands:

RequestAppend MockPipelineImplBase::Append(std::string /*key*/, std::string /*value*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestBitop MockPipelineImplBase::Bitop(
    BitOperation /*op*/,
    std::string /*dest_key*/,
    std::vector<std::string> /*src_keys*/
) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestDbsize MockPipelineImplBase::Dbsize(size_t /*shard*/) { AbortWithStacktrace("Redis method not mocked"); }

RequestDecr MockPipelineImplBase::Decr(std::string /*key*/) { AbortWithStacktrace("Redis method not mocked"); }

RequestDel MockPipelineImplBase::Del(std::string /*key*/) { AbortWithStacktrace("Redis method not mocked"); }

RequestDel MockPipelineImplBase::Del(std::vector<std::string> /*keys*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestUnlink MockPipelineImplBase::Unlink(std::string /*key*/) { AbortWithStacktrace("Redis method not mocked"); }

RequestUnlink MockPipelineImplBase::Unlink(std::vector<std::string> /*keys*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestExists MockPipelineImplBase::Exists(std::string /*key*/) { AbortWithStacktrace("Redis method not mocked"); }

RequestExists MockPipelineImplBase::Exists(std::vector<std::string> /*keys*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestExpire MockPipelineImplBase::Expire(std::string /*key*/, std::chrono::seconds /*ttl*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestExpire MockPipelineImplBase::Expire(
    std::string /*key*/,
    std::chrono::seconds /*ttl*/,
    ExpireOptions /*option*/
) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestGeoadd MockPipelineImplBase::Geoadd(std::string /*key*/, GeoaddArg /*point_member*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestGeoadd MockPipelineImplBase::Geoadd(std::string /*key*/, std::vector<GeoaddArg> /*point_members*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestGeopos MockPipelineImplBase::Geopos(std::string /*key*/, std::vector<std::string> /*members*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestGeoradius MockPipelineImplBase::Georadius(
    std::string /*key*/,
    Longitude /*lon*/,
    Latitude /*lat*/,
    double /*radius*/,
    const GeoradiusOptions& /*georadius_options*/
) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestGeosearch MockPipelineImplBase::Geosearch(
    std::string /*key*/,
    std::string /*member*/,
    double /*radius*/,
    const GeosearchOptions& /*geosearch_options*/
) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestGeosearch MockPipelineImplBase::Geosearch(
    std::string /*key*/,
    std::string /*member*/,
    BoxWidth /*width*/,
    BoxHeight /*height*/,
    const GeosearchOptions& /*geosearch_options*/
) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestGeosearch MockPipelineImplBase::Geosearch(
    std::string /*key*/,
    Longitude /*lon*/,
    Latitude /*lat*/,
    double /*radius*/,
    const GeosearchOptions& /*geosearch_options*/
) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestGeosearch MockPipelineImplBase::Geosearch(
    std::string /*key*/,
    Longitude /*lon*/,
    Latitude /*lat*/,
    BoxWidth /*width*/,
    BoxHeight /*height*/,
    const GeosearchOptions& /*geosearch_options*/
) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestGet MockPipelineImplBase::Get(std::string /*key*/) { AbortWithStacktrace("Redis method not mocked"); }

RequestGetset MockPipelineImplBase::Getset(std::string /*key*/, std::string /*value*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestHdel MockPipelineImplBase::Hdel(std::string /*key*/, std::string /*field*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestHdel MockPipelineImplBase::Hdel(std::string /*key*/, std::vector<std::string> /*fields*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestHexists MockPipelineImplBase::Hexists(std::string /*key*/, std::string /*field*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestHget MockPipelineImplBase::Hget(std::string /*key*/, std::string /*field*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestHgetall MockPipelineImplBase::Hgetall(std::string /*key*/) { AbortWithStacktrace("Redis method not mocked"); }

RequestHincrby MockPipelineImplBase::Hincrby(std::string /*key*/, std::string /*field*/, int64_t /*increment*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestHincrbyfloat MockPipelineImplBase::Hincrbyfloat(
    std::string /*key*/,
    std::string /*field*/,
    double /*increment*/
) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestHkeys MockPipelineImplBase::Hkeys(std::string /*key*/) { AbortWithStacktrace("Redis method not mocked"); }

RequestHlen MockPipelineImplBase::Hlen(std::string /*key*/) { AbortWithStacktrace("Redis method not mocked"); }

RequestHmget MockPipelineImplBase::Hmget(std::string /*key*/, std::vector<std::string> /*fields*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestHmset MockPipelineImplBase::Hmset(
    std::string /*key*/,
    std::vector<std::pair<std::string, std::string>> /*field_values*/
) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestHset MockPipelineImplBase::Hset(std::string /*key*/, std::string /*field*/, std::string /*value*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestHsetnx MockPipelineImplBase::Hsetnx(std::string /*key*/, std::string /*field*/, std::string /*value*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestHvals MockPipelineImplBase::Hvals(std::string /*key*/) { AbortWithStacktrace("Redis method not mocked"); }

RequestIncr MockPipelineImplBase::Incr(std::string /*key*/) { AbortWithStacktrace("Redis method not mocked"); }

RequestKeys MockPipelineImplBase::Keys(std::string /*keys_pattern*/, size_t /*shard*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestLindex MockPipelineImplBase::Lindex(std::string /*key*/, int64_t /*index*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestLlen MockPipelineImplBase::Llen(std::string /*key*/) { AbortWithStacktrace("Redis method not mocked"); }

RequestLpop MockPipelineImplBase::Lpop(std::string /*key*/) { AbortWithStacktrace("Redis method not mocked"); }

RequestLpush MockPipelineImplBase::Lpush(std::string /*key*/, std::string /*value*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestLpush MockPipelineImplBase::Lpush(std::string /*key*/, std::vector<std::string> /*values*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestLpushx MockPipelineImplBase::Lpushx(std::string /*key*/, std::string /*element*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestLrange MockPipelineImplBase::Lrange(std::string /*key*/, int64_t /*start*/, int64_t /*stop*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestLrem MockPipelineImplBase::Lrem(std::string /*key*/, int64_t /*count*/, std::string /*element*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestLtrim MockPipelineImplBase::Ltrim(std::string /*key*/, int64_t /*start*/, int64_t /*stop*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestMget MockPipelineImplBase::Mget(std::vector<std::string> /*keys*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestMset MockPipelineImplBase::Mset(std::vector<std::pair<std::string, std::string>> /*key_values*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestPersist MockPipelineImplBase::Persist(std::string /*key*/) { AbortWithStacktrace("Redis method not mocked"); }

RequestPexpire MockPipelineImplBase::Pexpire(std::string /*key*/, std::chrono::milliseconds /*ttl*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestPing MockPipelineImplBase::Ping(size_t /*shard*/) { AbortWithStacktrace("Redis method not mocked"); }

RequestPingMessage MockPipelineImplBase::PingMessage(size_t /*shard*/, std::string /*message*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestRename MockPipelineImplBase::Rename(std::string /*key*/, std::string /*new_key*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestRpop MockPipelineImplBase::Rpop(std::string /*key*/) { AbortWithStacktrace("Redis method not mocked"); }

RequestRpush MockPipelineImplBase::Rpush(std::string /*key*/, std::string /*value*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestRpush MockPipelineImplBase::Rpush(std::string /*key*/, std::vector<std::string> /*values*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestRpushx MockPipelineImplBase::Rpushx(std::string /*key*/, std::string /*element*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestSadd MockPipelineImplBase::Sadd(std::string /*key*/, std::string /*member*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestSadd MockPipelineImplBase::Sadd(std::string /*key*/, std::vector<std::string> /*members*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestScard MockPipelineImplBase::Scard(std::string /*key*/) { AbortWithStacktrace("Redis method not mocked"); }

RequestSet MockPipelineImplBase::Set(std::string /*key*/, std::string /*value*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestSet MockPipelineImplBase::Set(std::string /*key*/, std::string /*value*/, std::chrono::milliseconds /*ttl*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestSetIfExist MockPipelineImplBase::SetIfExist(std::string /*key*/, std::string /*value*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestSetIfExist MockPipelineImplBase::SetIfExist(
    std::string /*key*/,
    std::string /*value*/,
    std::chrono::milliseconds /*ttl*/
) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestSetIfNotExist MockPipelineImplBase::SetIfNotExist(std::string /*key*/, std::string /*value*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestSetIfNotExist MockPipelineImplBase::SetIfNotExist(
    std::string /*key*/,
    std::string /*value*/,
    std::chrono::milliseconds /*ttl*/
) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestSetIfNotExistOrGet MockPipelineImplBase::SetIfNotExistOrGet(std::string /*key*/, std::string /*value*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestSetIfNotExistOrGet MockPipelineImplBase::SetIfNotExistOrGet(
    std::string /*key*/,
    std::string /*value*/,
    std::chrono::milliseconds /*ttl*/
) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestSetex MockPipelineImplBase::Setex(
    std::string /*key*/,
    std::chrono::seconds /*seconds*/,
    std::string /*value*/
) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestSismember MockPipelineImplBase::Sismember(std::string /*key*/, std::string /*member*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestSmembers MockPipelineImplBase::Smembers(std::string /*key*/) { AbortWithStacktrace("Redis method not mocked"); }

RequestSrandmember MockPipelineImplBase::Srandmember(std::string /*key*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestSrandmembers MockPipelineImplBase::Srandmembers(std::string /*key*/, int64_t /*count*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestSrem MockPipelineImplBase::Srem(std::string /*key*/, std::string /*member*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestSrem MockPipelineImplBase::Srem(std::string /*key*/, std::vector<std::string> /*members*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestStrlen MockPipelineImplBase::Strlen(std::string /*key*/) { AbortWithStacktrace("Redis method not mocked"); }

RequestTime MockPipelineImplBase::Time(size_t /*shard*/) { AbortWithStacktrace("Redis method not mocked"); }

RequestTtl MockPipelineImplBase::Ttl(std::string /*key*/) { AbortWithStacktrace("Redis method not mocked"); }

RequestType MockPipelineImplBase::Type(std::string /*key*/) { AbortWithStacktrace("Redis method not mocked"); }

RequestZadd MockPipelineImplBase::Zadd(std::string /*key*/, double /*score*/, std::string /*member*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestZadd MockPipelineImplBase::Zadd(
    std::string /*key*/,
    double /*score*/,
    std::string /*member*/,
    const ZaddOptions& /*options*/
) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestZadd MockPipelineImplBase::Zadd(
    std::string /*key*/,
    std::vector<std::pair<double, std::string>> /*scored_members*/
) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestZadd MockPipelineImplBase::Zadd(
    std::string /*key*/,
    std::vector<std::pair<double, std::string>> /*scored_members*/,
    const ZaddOptions& /*options*/
) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestZaddIncr MockPipelineImplBase::ZaddIncr(std::string /*key*/, double /*score*/, std::string /*member*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestZaddIncrExisting MockPipelineImplBase::ZaddIncrExisting(
    std::string /*key*/,
    double /*score*/,
    std::string /*member*/
) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestZcard MockPipelineImplBase::Zcard(std::string /*key*/) { AbortWithStacktrace("Redis method not mocked"); }

RequestZcount MockPipelineImplBase::Zcount(std::string /*key*/, double /*min*/, double /*max*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestZrange MockPipelineImplBase::Zrange(std::string /*key*/, int64_t /*start*/, int64_t /*stop*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestZrangeWithScores MockPipelineImplBase::ZrangeWithScores(
    std::string /*key*/,
    int64_t /*start*/,
    int64_t /*stop*/
) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestZrangebyscore MockPipelineImplBase::Zrangebyscore(std::string /*key*/, double /*min*/, double /*max*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestZrangebyscore MockPipelineImplBase::Zrangebyscore(
    std::string /*key*/,
    std::string /*min*/,
    std::string /*max*/
) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestZrangebyscore MockPipelineImplBase::Zrangebyscore(
    std::string /*key*/,
    double /*min*/,
    double /*max*/,
    const RangeOptions& /*range_options*/
) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestZrangebyscore MockPipelineImplBase::Zrangebyscore(
    std::string /*key*/,
    std::string /*min*/,
    std::string /*max*/,
    const RangeOptions& /*range_options*/
) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestZrangebyscoreWithScores MockPipelineImplBase::ZrangebyscoreWithScores(
    std::string /*key*/,
    double /*min*/,
    double /*max*/
) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestZrangebyscoreWithScores MockPipelineImplBase::ZrangebyscoreWithScores(
    std::string /*key*/,
    std::string /*min*/,
    std::string /*max*/
) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestZrangebyscoreWithScores MockPipelineImplBase::ZrangebyscoreWithScores(
    std::string /*key*/,
    double /*min*/,
    double /*max*/,
    const RangeOptions& /*range_options*/
) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestZrangebyscoreWithScores MockPipelineImplBase::ZrangebyscoreWithScores(
    std::string /*key*/,
    std::string /*min*/,
    std::string /*max*/,
    const RangeOptions& /*range_options*/
) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestZrem MockPipelineImplBase::Zrem(std::string /*key*/, std::string /*member*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestZrem MockPipelineImplBase::Zrem(std::string /*key*/, std::vector<std::string> /*members*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestZremrangebyrank MockPipelineImplBase::Zremrangebyrank(
    std::string /*key*/,
    int64_t /*start*/,
    int64_t /*stop*/
) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestZremrangebyscore MockPipelineImplBase::Zremrangebyscore(std::string /*key*/, double /*min*/, double /*max*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestZremrangebyscore MockPipelineImplBase::Zremrangebyscore(
    std::string /*key*/,
    std::string /*min*/,
    std::string /*max*/
) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestZscore MockPipelineImplBase::Zscore(std::string /*key*/, std::string /*member*/) {
    AbortWithStacktrace("Redis method not mocked");
}

// end of redis commands

}  // namespace storages::redis

USERVER_NAMESPACE_END
