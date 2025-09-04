#include <userver/storages/redis/mock_client_base.hpp>

#include <userver/utils/assert.hpp>

#include <userver/storages/redis/mock_transaction.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

using utils::AbortWithStacktrace;

namespace {
constexpr std::string_view kNotMocked{"Redis method is not mocked"};
}

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
    AbortWithStacktrace(kNotMocked);
}

RequestBitop MockClientBase::Bitop(
    BitOperation /*op*/,
    std::string /*dest_key*/,
    std::vector<std::string> /*src_keys*/,
    const CommandControl& /*command_control*/
) {
    AbortWithStacktrace(kNotMocked);
}

RequestDbsize MockClientBase::Dbsize(size_t /*shard*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace(kNotMocked);
}

RequestDecr MockClientBase::Decr(std::string /*key*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace(kNotMocked);
}

RequestDel MockClientBase::Del(std::string /*key*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace(kNotMocked);
}

RequestDel MockClientBase::Del(std::vector<std::string> /*keys*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace(kNotMocked);
}

RequestUnlink MockClientBase::Unlink(std::string /*key*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace(kNotMocked);
}

RequestUnlink MockClientBase::Unlink(std::vector<std::string> /*keys*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace(kNotMocked);
}

RequestEvalCommon MockClientBase::EvalCommon(
    std::string /*script*/,
    std::vector<std::string> /*keys*/,
    std::vector<std::string> /*args*/,
    const CommandControl& /*command_control*/
) {
    AbortWithStacktrace(kNotMocked);
}

RequestEvalShaCommon MockClientBase::EvalShaCommon(
    std::string /*script*/,
    std::vector<std::string> /*keys*/,
    std::vector<std::string> /*args*/,
    const CommandControl& /*command_control*/
) {
    AbortWithStacktrace(kNotMocked);
}

RequestGenericCommon MockClientBase::GenericCommon(
    std::string /*command*/,
    std::vector<std::string> /*args*/,
    size_t /*key_index*/,
    const CommandControl& /*command_control*/
) {
    AbortWithStacktrace(kNotMocked);
}

RequestScriptLoad
MockClientBase::ScriptLoad(std::string /*script*/, size_t /*shard*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace(kNotMocked);
}

RequestExists MockClientBase::Exists(std::string /*key*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace(kNotMocked);
}

RequestExists MockClientBase::Exists(std::vector<std::string> /*keys*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace(kNotMocked);
}

RequestExpire
MockClientBase::Expire(std::string /*key*/, std::chrono::seconds /*ttl*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace(kNotMocked);
}

RequestGeoadd
MockClientBase::Geoadd(std::string /*key*/, GeoaddArg /*point_member*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace(kNotMocked);
}

RequestGeoadd MockClientBase::
    Geoadd(std::string /*key*/, std::vector<GeoaddArg> /*point_members*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace(kNotMocked);
}

RequestGeoradius MockClientBase::Georadius(
    std::string /*key*/,
    Longitude /*lon*/,
    Latitude /*lat*/,
    double /*radius*/,
    const GeoradiusOptions& /*georadius_options*/,
    const CommandControl& /*command_control*/
) {
    AbortWithStacktrace(kNotMocked);
}

RequestGeosearch MockClientBase::Geosearch(
    std::string /*key*/,
    std::string /*member*/,
    double /*radius*/,
    const GeosearchOptions& /*geosearch_options*/,
    const CommandControl& /*command_control*/
) {
    AbortWithStacktrace(kNotMocked);
}

RequestGeosearch MockClientBase::Geosearch(
    std::string /*key*/,
    std::string /*member*/,
    BoxWidth /*width*/,
    BoxHeight /*height*/,
    const GeosearchOptions& /*geosearch_options*/,
    const CommandControl& /*command_control*/
) {
    AbortWithStacktrace(kNotMocked);
}

RequestGeosearch MockClientBase::Geosearch(
    std::string /*key*/,
    Longitude /*lon*/,
    Latitude /*lat*/,
    double /*radius*/,
    const GeosearchOptions& /*geosearch_options*/,
    const CommandControl& /*command_control*/
) {
    AbortWithStacktrace(kNotMocked);
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
    AbortWithStacktrace(kNotMocked);
}

RequestGet MockClientBase::Get(std::string /*key*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace(kNotMocked);
}

RequestGetset
MockClientBase::Getset(std::string /*key*/, std::string /*value*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace(kNotMocked);
}

RequestHdel
MockClientBase::Hdel(std::string /*key*/, std::string /*field*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace(kNotMocked);
}

RequestHdel MockClientBase::
    Hdel(std::string /*key*/, std::vector<std::string> /*fields*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace(kNotMocked);
}

RequestHexists
MockClientBase::Hexists(std::string /*key*/, std::string /*field*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace(kNotMocked);
}

RequestHget
MockClientBase::Hget(std::string /*key*/, std::string /*field*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace(kNotMocked);
}

RequestHgetall MockClientBase::Hgetall(std::string /*key*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace(kNotMocked);
}

RequestHincrby MockClientBase::Hincrby(
    std::string /*key*/,
    std::string /*field*/,
    int64_t /*increment*/,
    const CommandControl& /*command_control*/
) {
    AbortWithStacktrace(kNotMocked);
}

RequestHincrbyfloat MockClientBase::Hincrbyfloat(
    std::string /*key*/,
    std::string /*field*/,
    double /*increment*/,
    const CommandControl& /*command_control*/
) {
    AbortWithStacktrace(kNotMocked);
}

RequestHkeys MockClientBase::Hkeys(std::string /*key*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace(kNotMocked);
}

RequestHlen MockClientBase::Hlen(std::string /*key*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace(kNotMocked);
}

RequestHmget MockClientBase::
    Hmget(std::string /*key*/, std::vector<std::string> /*fields*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace(kNotMocked);
}

RequestHmset MockClientBase::Hmset(
    std::string /*key*/,
    std::vector<std::pair<std::string, std::string>> /*field_values*/,
    const CommandControl& /*command_control*/
) {
    AbortWithStacktrace(kNotMocked);
}

ScanRequest<ScanTag::kHscan> MockClientBase::Hscan(
    std::string /*key*/,
    HscanOptions /*options*/,
    const CommandControl& /*command_control*/
) {
    AbortWithStacktrace(kNotMocked);
}

RequestHset MockClientBase::
    Hset(std::string /*key*/, std::string /*field*/, std::string /*value*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace(kNotMocked);
}

RequestHsetnx MockClientBase::Hsetnx(
    std::string /*key*/,
    std::string /*field*/,
    std::string /*value*/,
    const CommandControl& /*command_control*/
) {
    AbortWithStacktrace(kNotMocked);
}

RequestHvals MockClientBase::Hvals(std::string /*key*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace(kNotMocked);
}

RequestIncr MockClientBase::Incr(std::string /*key*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace(kNotMocked);
}

RequestKeys
MockClientBase::Keys(std::string /*keys_pattern*/, size_t /*shard*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace(kNotMocked);
}

RequestLindex
MockClientBase::Lindex(std::string /*key*/, int64_t /*index*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace(kNotMocked);
}

RequestLlen MockClientBase::Llen(std::string /*key*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace(kNotMocked);
}

RequestLpop MockClientBase::Lpop(std::string /*key*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace(kNotMocked);
}

RequestLpush
MockClientBase::Lpush(std::string /*key*/, std::string /*value*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace(kNotMocked);
}

RequestLpush MockClientBase::
    Lpush(std::string /*key*/, std::vector<std::string> /*values*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace(kNotMocked);
}

RequestLpushx
MockClientBase::Lpushx(std::string /*key*/, std::string /*element*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace(kNotMocked);
}

RequestLrange MockClientBase::
    Lrange(std::string /*key*/, int64_t /*start*/, int64_t /*stop*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace(kNotMocked);
}

RequestLrem MockClientBase::
    Lrem(std::string /*key*/, int64_t /*count*/, std::string /*element*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace(kNotMocked);
}

RequestLtrim MockClientBase::
    Ltrim(std::string /*key*/, int64_t /*start*/, int64_t /*stop*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace(kNotMocked);
}

RequestMget MockClientBase::Mget(std::vector<std::string> /*keys*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace(kNotMocked);
}

RequestMset MockClientBase::
    Mset(std::vector<std::pair<std::string, std::string>> /*key_values*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace(kNotMocked);
}

RequestPersist MockClientBase::Persist(std::string /*key*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace(kNotMocked);
}

RequestPexpire MockClientBase::
    Pexpire(std::string /*key*/, std::chrono::milliseconds /*ttl*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace(kNotMocked);
}

RequestPing MockClientBase::Ping(size_t /*shard*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace(kNotMocked);
}

RequestPingMessage
MockClientBase::Ping(size_t /*shard*/, std::string /*message*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace(kNotMocked);
}

void MockClientBase::Publish(
    std::string /*channel*/,
    std::string /*message*/,
    const CommandControl& /*command_control*/,
    PubShard /*policy*/
) {
    AbortWithStacktrace(kNotMocked);
}

void MockClientBase::
    Spublish(std::string /*channel*/, std::string /*message*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace(kNotMocked);
}

RequestRename
MockClientBase::Rename(std::string /*key*/, std::string /*new_key*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace(kNotMocked);
}

RequestRpop MockClientBase::Rpop(std::string /*key*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace(kNotMocked);
}

RequestRpush
MockClientBase::Rpush(std::string /*key*/, std::string /*value*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace(kNotMocked);
}

RequestRpush MockClientBase::
    Rpush(std::string /*key*/, std::vector<std::string> /*values*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace(kNotMocked);
}

RequestRpushx
MockClientBase::Rpushx(std::string /*key*/, std::string /*element*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace(kNotMocked);
}

RequestSadd
MockClientBase::Sadd(std::string /*key*/, std::string /*member*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace(kNotMocked);
}

RequestSadd MockClientBase::
    Sadd(std::string /*key*/, std::vector<std::string> /*members*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace(kNotMocked);
}

ScanRequest<ScanTag::kScan>
MockClientBase::Scan(size_t /*shard*/, ScanOptions /*options*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace(kNotMocked);
}

RequestScard MockClientBase::Scard(std::string /*key*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace(kNotMocked);
}

RequestSet MockClientBase::Set(std::string /*key*/, std::string /*value*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace(kNotMocked);
}

RequestSet MockClientBase::
    Set(std::string /*key*/,
        std::string /*value*/,
        std::chrono::milliseconds /*ttl*/,
        const CommandControl& /*command_control*/) {
    AbortWithStacktrace(kNotMocked);
}

RequestSetIfExist
MockClientBase::SetIfExist(std::string /*key*/, std::string /*value*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace(kNotMocked);
}

RequestSetIfExist MockClientBase::SetIfExist(
    std::string /*key*/,
    std::string /*value*/,
    std::chrono::milliseconds /*ttl*/,
    const CommandControl& /*command_control*/
) {
    AbortWithStacktrace(kNotMocked);
}

RequestSetIfNotExist
MockClientBase::SetIfNotExist(std::string /*key*/, std::string /*value*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace(kNotMocked);
}

RequestSetIfNotExist MockClientBase::SetIfNotExist(
    std::string /*key*/,
    std::string /*value*/,
    std::chrono::milliseconds /*ttl*/,
    const CommandControl& /*command_control*/
) {
    AbortWithStacktrace(kNotMocked);
}

RequestSetIfNotExistOrGet MockClientBase::
    SetIfNotExistOrGet(std::string /*key*/, std::string /*value*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace(kNotMocked);
}

RequestSetIfNotExistOrGet MockClientBase::SetIfNotExistOrGet(
    std::string /*key*/,
    std::string /*value*/,
    std::chrono::milliseconds /*ttl*/,
    const CommandControl& /*command_control*/
) {
    AbortWithStacktrace(kNotMocked);
}

RequestSetex MockClientBase::Setex(
    std::string /*key*/,
    std::chrono::seconds /*seconds*/,
    std::string /*value*/,
    const CommandControl& /*command_control*/
) {
    AbortWithStacktrace(kNotMocked);
}

RequestSismember
MockClientBase::Sismember(std::string /*key*/, std::string /*member*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace(kNotMocked);
}

RequestSmembers MockClientBase::Smembers(std::string /*key*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace(kNotMocked);
}

RequestSrandmember MockClientBase::Srandmember(std::string /*key*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace(kNotMocked);
}

RequestSrandmembers
MockClientBase::Srandmembers(std::string /*key*/, int64_t /*count*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace(kNotMocked);
}

RequestSrem
MockClientBase::Srem(std::string /*key*/, std::string /*member*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace(kNotMocked);
}

RequestSrem MockClientBase::
    Srem(std::string /*key*/, std::vector<std::string> /*members*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace(kNotMocked);
}

ScanRequest<ScanTag::kSscan> MockClientBase::Sscan(
    std::string /*key*/,
    SscanOptions /*options*/,
    const CommandControl& /*command_control*/
) {
    AbortWithStacktrace(kNotMocked);
}

RequestStrlen MockClientBase::Strlen(std::string /*key*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace(kNotMocked);
}

RequestTime MockClientBase::Time(size_t /*shard*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace(kNotMocked);
}

RequestTtl MockClientBase::Ttl(std::string /*key*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace(kNotMocked);
}

RequestType MockClientBase::Type(std::string /*key*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace(kNotMocked);
}

RequestZadd MockClientBase::
    Zadd(std::string /*key*/, double /*score*/, std::string /*member*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace(kNotMocked);
}

RequestZadd MockClientBase::Zadd(
    std::string /*key*/,
    double /*score*/,
    std::string /*member*/,
    const ZaddOptions& /*options*/,
    const CommandControl& /*command_control*/
) {
    AbortWithStacktrace(kNotMocked);
}

RequestZadd MockClientBase::Zadd(
    std::string /*key*/,
    std::vector<std::pair<double, std::string>> /*scored_members*/,
    const CommandControl& /*command_control*/
) {
    AbortWithStacktrace(kNotMocked);
}

RequestZadd MockClientBase::Zadd(
    std::string /*key*/,
    std::vector<std::pair<double, std::string>> /*scored_members*/,
    const ZaddOptions& /*options*/,
    const CommandControl& /*command_control*/
) {
    AbortWithStacktrace(kNotMocked);
}

RequestZaddIncr MockClientBase::
    ZaddIncr(std::string /*key*/, double /*score*/, std::string /*member*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace(kNotMocked);
}

RequestZaddIncrExisting MockClientBase::ZaddIncrExisting(
    std::string /*key*/,
    double /*score*/,
    std::string /*member*/,
    const CommandControl& /*command_control*/
) {
    AbortWithStacktrace(kNotMocked);
}

RequestZcard MockClientBase::Zcard(std::string /*key*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace(kNotMocked);
}

RequestZcount
MockClientBase::Zcount(std::string /*key*/, double /*min*/, double /*max*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace(kNotMocked);
}

RequestZrange MockClientBase::
    Zrange(std::string /*key*/, int64_t /*start*/, int64_t /*stop*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace(kNotMocked);
}

RequestZrangeWithScores MockClientBase::ZrangeWithScores(
    std::string /*key*/,
    int64_t /*start*/,
    int64_t /*stop*/,
    const CommandControl& /*command_control*/
) {
    AbortWithStacktrace(kNotMocked);
}

RequestZrangebyscore MockClientBase::
    Zrangebyscore(std::string /*key*/, double /*min*/, double /*max*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace(kNotMocked);
}

RequestZrangebyscore MockClientBase::Zrangebyscore(
    std::string /*key*/,
    std::string /*min*/,
    std::string /*max*/,
    const CommandControl& /*command_control*/
) {
    AbortWithStacktrace(kNotMocked);
}

RequestZrangebyscore MockClientBase::Zrangebyscore(
    std::string /*key*/,
    double /*min*/,
    double /*max*/,
    const RangeOptions& /*range_options*/,
    const CommandControl& /*command_control*/
) {
    AbortWithStacktrace(kNotMocked);
}

RequestZrangebyscore MockClientBase::Zrangebyscore(
    std::string /*key*/,
    std::string /*min*/,
    std::string /*max*/,
    const RangeOptions& /*range_options*/,
    const CommandControl& /*command_control*/
) {
    AbortWithStacktrace(kNotMocked);
}

RequestZrangebyscoreWithScores MockClientBase::ZrangebyscoreWithScores(
    std::string /*key*/,
    double /*min*/,
    double /*max*/,
    const CommandControl& /*command_control*/
) {
    AbortWithStacktrace(kNotMocked);
}

RequestZrangebyscoreWithScores MockClientBase::ZrangebyscoreWithScores(
    std::string /*key*/,
    std::string /*min*/,
    std::string /*max*/,
    const CommandControl& /*command_control*/
) {
    AbortWithStacktrace(kNotMocked);
}

RequestZrangebyscoreWithScores MockClientBase::ZrangebyscoreWithScores(
    std::string /*key*/,
    double /*min*/,
    double /*max*/,
    const RangeOptions& /*range_options*/,
    const CommandControl& /*command_control*/
) {
    AbortWithStacktrace(kNotMocked);
}

RequestZrangebyscoreWithScores MockClientBase::ZrangebyscoreWithScores(
    std::string /*key*/,
    std::string /*min*/,
    std::string /*max*/,
    const RangeOptions& /*range_options*/,
    const CommandControl& /*command_control*/
) {
    AbortWithStacktrace(kNotMocked);
}

RequestZrem
MockClientBase::Zrem(std::string /*key*/, std::string /*member*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace(kNotMocked);
}

RequestZrem MockClientBase::
    Zrem(std::string /*key*/, std::vector<std::string> /*members*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace(kNotMocked);
}

RequestZremrangebyrank MockClientBase::Zremrangebyrank(
    std::string /*key*/,
    int64_t /*start*/,
    int64_t /*stop*/,
    const CommandControl& /*command_control*/
) {
    AbortWithStacktrace(kNotMocked);
}

RequestZremrangebyscore MockClientBase::
    Zremrangebyscore(std::string /*key*/, double /*min*/, double /*max*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace(kNotMocked);
}

RequestZremrangebyscore MockClientBase::Zremrangebyscore(
    std::string /*key*/,
    std::string /*min*/,
    std::string /*max*/,
    const CommandControl& /*command_control*/
) {
    AbortWithStacktrace(kNotMocked);
}

ScanRequest<ScanTag::kZscan> MockClientBase::Zscan(
    std::string /*key*/,
    ZscanOptions /*options*/,
    const CommandControl& /*command_control*/
) {
    AbortWithStacktrace(kNotMocked);
}

RequestZscore
MockClientBase::Zscore(std::string /*key*/, std::string /*member*/, const CommandControl& /*command_control*/) {
    AbortWithStacktrace(kNotMocked);
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
