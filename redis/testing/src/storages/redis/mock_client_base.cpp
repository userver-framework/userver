#include <userver/storages/redis/mock_client_base.hpp>

#include <userver/utils/assert.hpp>

#include <userver/storages/redis/mock_transaction.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

using utils::AbortWithStacktrace;

MockClientBase::MockClientBase()
    : mock_transaction_impl_creator_(std::make_unique<MockTransactionImplCreator<MockTransactionImplBase>>()) {}

MockClientBase::MockClientBase(
    std::shared_ptr<MockTransactionImplCreatorBase> mock_transaction_impl_creator,
    std::optional<size_t> force_shard_idx
)
    : mock_transaction_impl_creator_(std::move(mock_transaction_impl_creator)), force_shard_idx_(force_shard_idx) {}

MockClientBase::~MockClientBase() = default;

void MockClientBase::WaitConnectedOnce(RedisWaitConnected) {}

size_t MockClientBase::ShardsCount() const { return 1; }

bool MockClientBase::IsInClusterMode() const { return false; }

size_t MockClientBase::ShardByKey(const std::string& /*key*/) const { return 0; }

// redis commands:

RequestAppend
MockClientBase::Append(std::string /*key*/, std::string /*value*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestBitop MockClientBase::Bitop(
    BitOperation /*op*/,
    std::string /*dest_key*/,
    std::vector<std::string> /*src_keys*/,
    const CommandControl& /*command_control*/
) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestDbsize MockClientBase::Dbsize(size_t /*shard*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestDecr MockClientBase::Decr(std::string /*key*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestDel MockClientBase::Del(std::string /*key*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestDel MockClientBase::Del(std::vector<std::string> /*keys*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestUnlink MockClientBase::Unlink(std::string /*key*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestUnlink MockClientBase::Unlink(std::vector<std::string> /*keys*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestEvalCommon MockClientBase::EvalCommon(
    std::string /*script*/,
    std::vector<std::string> /*keys*/,
    std::vector<std::string> /*args*/,
    const CommandControl& /*command_control*/
) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestEvalShaCommon MockClientBase::EvalShaCommon(
    std::string /*script*/,
    std::vector<std::string> /*keys*/,
    std::vector<std::string> /*args*/,
    const CommandControl& /*command_control*/
) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestScriptLoad
MockClientBase::ScriptLoad(std::string /*script*/, size_t /*shard*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestExists MockClientBase::Exists(std::string /*key*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestExists MockClientBase::Exists(std::vector<std::string> /*keys*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestExpire
MockClientBase::Expire(std::string /*key*/, std::chrono::seconds /*ttl*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestGeoadd
MockClientBase::Geoadd(std::string /*key*/, GeoaddArg /*point_member*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestGeoadd MockClientBase::
    Geoadd(std::string /*key*/, std::vector<GeoaddArg> /*point_members*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestGeoradius MockClientBase::Georadius(
    std::string /*key*/,
    Longitude /*lon*/,
    Latitude /*lat*/,
    double /*radius*/,
    const GeoradiusOptions& /*georadius_options*/,
    const CommandControl& /*command_control*/
) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestGeosearch MockClientBase::Geosearch(
    std::string /*key*/,
    std::string /*member*/,
    double /*radius*/,
    const GeosearchOptions& /*geosearch_options*/,
    const CommandControl& /*command_control*/
) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestGeosearch MockClientBase::Geosearch(
    std::string /*key*/,
    std::string /*member*/,
    BoxWidth /*width*/,
    BoxHeight /*height*/,
    const GeosearchOptions& /*geosearch_options*/,
    const CommandControl& /*command_control*/
) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestGeosearch MockClientBase::Geosearch(
    std::string /*key*/,
    Longitude /*lon*/,
    Latitude /*lat*/,
    double /*radius*/,
    const GeosearchOptions& /*geosearch_options*/,
    const CommandControl& /*command_control*/
) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestGeosearch MockClientBase::Geosearch(
    std::string /*key*/,
    Longitude /*lon*/,
    Latitude /*lat*/,
    BoxWidth /*width*/,
    BoxHeight /*height*/,
    const GeosearchOptions& /*geosearch_options*/,
    const CommandControl& /*command_control*/
) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestGet MockClientBase::Get(std::string /*key*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestGetset
MockClientBase::Getset(std::string /*key*/, std::string /*value*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestHdel
MockClientBase::Hdel(std::string /*key*/, std::string /*field*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestHdel MockClientBase::
    Hdel(std::string /*key*/, std::vector<std::string> /*fields*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestHexists
MockClientBase::Hexists(std::string /*key*/, std::string /*field*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestHget
MockClientBase::Hget(std::string /*key*/, std::string /*field*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestHgetall MockClientBase::Hgetall(std::string /*key*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestHincrby MockClientBase::Hincrby(
    std::string /*key*/,
    std::string /*field*/,
    int64_t /*increment*/,
    const CommandControl& /*command_control*/
) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestHincrbyfloat MockClientBase::Hincrbyfloat(
    std::string /*key*/,
    std::string /*field*/,
    double /*increment*/,
    const CommandControl& /*command_control*/
) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestHkeys MockClientBase::Hkeys(std::string /*key*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestHlen MockClientBase::Hlen(std::string /*key*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestHmget MockClientBase::
    Hmget(std::string /*key*/, std::vector<std::string> /*fields*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestHmset MockClientBase::Hmset(
    std::string /*key*/,
    std::vector<std::pair<std::string, std::string>> /*field_values*/,
    const CommandControl& /*command_control*/
) {
    AbortWithStacktrace("Redis method not mocked");
}

ScanRequest<ScanTag::kHscan> MockClientBase::Hscan(
    std::string /*key*/,
    HscanOptions /*options*/,
    const CommandControl& /*command_control*/
) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestHset MockClientBase::
    Hset(std::string /*key*/, std::string /*field*/, std::string /*value*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestHsetnx MockClientBase::Hsetnx(
    std::string /*key*/,
    std::string /*field*/,
    std::string /*value*/,
    const CommandControl& /*command_control*/
) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestHvals MockClientBase::Hvals(std::string /*key*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestIncr MockClientBase::Incr(std::string /*key*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestKeys
MockClientBase::Keys(std::string /*keys_pattern*/, size_t /*shard*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestLindex
MockClientBase::Lindex(std::string /*key*/, int64_t /*index*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestLlen MockClientBase::Llen(std::string /*key*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestLpop MockClientBase::Lpop(std::string /*key*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestLpush
MockClientBase::Lpush(std::string /*key*/, std::string /*value*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestLpush MockClientBase::
    Lpush(std::string /*key*/, std::vector<std::string> /*values*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestLpushx
MockClientBase::Lpushx(std::string /*key*/, std::string /*element*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestLrange MockClientBase::
    Lrange(std::string /*key*/, int64_t /*start*/, int64_t /*stop*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestLrem MockClientBase::
    Lrem(std::string /*key*/, int64_t /*count*/, std::string /*element*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestLtrim MockClientBase::
    Ltrim(std::string /*key*/, int64_t /*start*/, int64_t /*stop*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestMget MockClientBase::Mget(std::vector<std::string> /*keys*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestMset MockClientBase::
    Mset(std::vector<std::pair<std::string, std::string>> /*key_values*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestPersist MockClientBase::Persist(std::string /*key*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestPexpire MockClientBase::
    Pexpire(std::string /*key*/, std::chrono::milliseconds /*ttl*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestPing MockClientBase::Ping(size_t /*shard*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestPingMessage
MockClientBase::Ping(size_t /*shard*/, std::string /*message*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace("Redis method not mocked");
}

void MockClientBase::Publish(
    std::string /*channel*/,
    std::string /*message*/,
    const CommandControl& /*command_control*/,
    PubShard /*policy*/
) {
    AbortWithStacktrace("Redis method not mocked");
}

void MockClientBase::
    Spublish(std::string /*channel*/, std::string /*message*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestRename
MockClientBase::Rename(std::string /*key*/, std::string /*new_key*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestRpop MockClientBase::Rpop(std::string /*key*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestRpush
MockClientBase::Rpush(std::string /*key*/, std::string /*value*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestRpush MockClientBase::
    Rpush(std::string /*key*/, std::vector<std::string> /*values*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestRpushx
MockClientBase::Rpushx(std::string /*key*/, std::string /*element*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestSadd
MockClientBase::Sadd(std::string /*key*/, std::string /*member*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestSadd MockClientBase::
    Sadd(std::string /*key*/, std::vector<std::string> /*members*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace("Redis method not mocked");
}

ScanRequest<ScanTag::kScan>
MockClientBase::Scan(size_t /*shard*/, ScanOptions /*options*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestScard MockClientBase::Scard(std::string /*key*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestSet MockClientBase::Set(std::string /*key*/, std::string /*value*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestSet MockClientBase::
    Set(std::string /*key*/,
        std::string /*value*/,
        std::chrono::milliseconds /*ttl*/,
        const CommandControl& /*command_control*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestSetIfExist
MockClientBase::SetIfExist(std::string /*key*/, std::string /*value*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestSetIfExist MockClientBase::SetIfExist(
    std::string /*key*/,
    std::string /*value*/,
    std::chrono::milliseconds /*ttl*/,
    const CommandControl& /*command_control*/
) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestSetIfNotExist
MockClientBase::SetIfNotExist(std::string /*key*/, std::string /*value*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestSetIfNotExist MockClientBase::SetIfNotExist(
    std::string /*key*/,
    std::string /*value*/,
    std::chrono::milliseconds /*ttl*/,
    const CommandControl& /*command_control*/
) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestSetIfNotExistOrGet MockClientBase::
    SetIfNotExistOrGet(std::string /*key*/, std::string /*value*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestSetIfNotExistOrGet MockClientBase::SetIfNotExistOrGet(
    std::string /*key*/,
    std::string /*value*/,
    std::chrono::milliseconds /*ttl*/,
    const CommandControl& /*command_control*/
) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestSetex MockClientBase::Setex(
    std::string /*key*/,
    std::chrono::seconds /*seconds*/,
    std::string /*value*/,
    const CommandControl& /*command_control*/
) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestSismember
MockClientBase::Sismember(std::string /*key*/, std::string /*member*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestSmembers MockClientBase::Smembers(std::string /*key*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestSrandmember MockClientBase::Srandmember(std::string /*key*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestSrandmembers
MockClientBase::Srandmembers(std::string /*key*/, int64_t /*count*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestSrem
MockClientBase::Srem(std::string /*key*/, std::string /*member*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestSrem MockClientBase::
    Srem(std::string /*key*/, std::vector<std::string> /*members*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace("Redis method not mocked");
}

ScanRequest<ScanTag::kSscan> MockClientBase::Sscan(
    std::string /*key*/,
    SscanOptions /*options*/,
    const CommandControl& /*command_control*/
) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestStrlen MockClientBase::Strlen(std::string /*key*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestTime MockClientBase::Time(size_t /*shard*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestTtl MockClientBase::Ttl(std::string /*key*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestType MockClientBase::Type(std::string /*key*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestZadd MockClientBase::
    Zadd(std::string /*key*/, double /*score*/, std::string /*member*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestZadd MockClientBase::Zadd(
    std::string /*key*/,
    double /*score*/,
    std::string /*member*/,
    const ZaddOptions& /*options*/,
    const CommandControl& /*command_control*/
) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestZadd MockClientBase::Zadd(
    std::string /*key*/,
    std::vector<std::pair<double, std::string>> /*scored_members*/,
    const CommandControl& /*command_control*/
) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestZadd MockClientBase::Zadd(
    std::string /*key*/,
    std::vector<std::pair<double, std::string>> /*scored_members*/,
    const ZaddOptions& /*options*/,
    const CommandControl& /*command_control*/
) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestZaddIncr MockClientBase::
    ZaddIncr(std::string /*key*/, double /*score*/, std::string /*member*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestZaddIncrExisting MockClientBase::ZaddIncrExisting(
    std::string /*key*/,
    double /*score*/,
    std::string /*member*/,
    const CommandControl& /*command_control*/
) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestZcard MockClientBase::Zcard(std::string /*key*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestZcount
MockClientBase::Zcount(std::string /*key*/, double /*min*/, double /*max*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestZrange MockClientBase::
    Zrange(std::string /*key*/, int64_t /*start*/, int64_t /*stop*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestZrangeWithScores MockClientBase::ZrangeWithScores(
    std::string /*key*/,
    int64_t /*start*/,
    int64_t /*stop*/,
    const CommandControl& /*command_control*/
) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestZrangebyscore MockClientBase::
    Zrangebyscore(std::string /*key*/, double /*min*/, double /*max*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestZrangebyscore MockClientBase::Zrangebyscore(
    std::string /*key*/,
    std::string /*min*/,
    std::string /*max*/,
    const CommandControl& /*command_control*/
) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestZrangebyscore MockClientBase::Zrangebyscore(
    std::string /*key*/,
    double /*min*/,
    double /*max*/,
    const RangeOptions& /*range_options*/,
    const CommandControl& /*command_control*/
) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestZrangebyscore MockClientBase::Zrangebyscore(
    std::string /*key*/,
    std::string /*min*/,
    std::string /*max*/,
    const RangeOptions& /*range_options*/,
    const CommandControl& /*command_control*/
) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestZrangebyscoreWithScores MockClientBase::ZrangebyscoreWithScores(
    std::string /*key*/,
    double /*min*/,
    double /*max*/,
    const CommandControl& /*command_control*/
) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestZrangebyscoreWithScores MockClientBase::ZrangebyscoreWithScores(
    std::string /*key*/,
    std::string /*min*/,
    std::string /*max*/,
    const CommandControl& /*command_control*/
) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestZrangebyscoreWithScores MockClientBase::ZrangebyscoreWithScores(
    std::string /*key*/,
    double /*min*/,
    double /*max*/,
    const RangeOptions& /*range_options*/,
    const CommandControl& /*command_control*/
) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestZrangebyscoreWithScores MockClientBase::ZrangebyscoreWithScores(
    std::string /*key*/,
    std::string /*min*/,
    std::string /*max*/,
    const RangeOptions& /*range_options*/,
    const CommandControl& /*command_control*/
) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestZrem
MockClientBase::Zrem(std::string /*key*/, std::string /*member*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestZrem MockClientBase::
    Zrem(std::string /*key*/, std::vector<std::string> /*members*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestZremrangebyrank MockClientBase::Zremrangebyrank(
    std::string /*key*/,
    int64_t /*start*/,
    int64_t /*stop*/,
    const CommandControl& /*command_control*/
) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestZremrangebyscore MockClientBase::
    Zremrangebyscore(std::string /*key*/, double /*min*/, double /*max*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestZremrangebyscore MockClientBase::Zremrangebyscore(
    std::string /*key*/,
    std::string /*min*/,
    std::string /*max*/,
    const CommandControl& /*command_control*/
) {
    AbortWithStacktrace("Redis method not mocked");
}

ScanRequest<ScanTag::kZscan> MockClientBase::Zscan(
    std::string /*key*/,
    ZscanOptions /*options*/,
    const CommandControl& /*command_control*/
) {
    AbortWithStacktrace("Redis method not mocked");
}

RequestZscore
MockClientBase::Zscore(std::string /*key*/, std::string /*member*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace("Redis method not mocked");
}

// end of redis commands

TransactionPtr MockClientBase::Multi() {
    UASSERT_MSG(!!mock_transaction_impl_creator_, "MockTransactionImpl type not set");
    return std::make_unique<MockTransaction>(shared_from_this(), (*mock_transaction_impl_creator_)());
}

TransactionPtr MockClientBase::Multi(Transaction::CheckShards check_shards) {
    UASSERT_MSG(!!mock_transaction_impl_creator_, "MockTransactionImpl type not set");
    return std::make_unique<MockTransaction>(shared_from_this(), (*mock_transaction_impl_creator_)(), check_shards);
}

}  // namespace storages::redis

USERVER_NAMESPACE_END
