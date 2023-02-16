#include <userver/storages/redis/mock_client_base.hpp>

#include <userver/utils/assert.hpp>

#include <userver/storages/redis/mock_transaction.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

MockClientBase::MockClientBase()
    : mock_transaction_impl_creator_(
          std::make_unique<
              MockTransactionImplCreator<MockTransactionImplBase>>()) {}

MockClientBase::MockClientBase(std::shared_ptr<MockTransactionImplCreatorBase>
                                   mock_transaction_impl_creator,
                               std::optional<size_t> force_shard_idx)
    : mock_transaction_impl_creator_(std::move(mock_transaction_impl_creator)),
      force_shard_idx_(force_shard_idx) {}

MockClientBase::~MockClientBase() = default;

void MockClientBase::WaitConnectedOnce(
    USERVER_NAMESPACE::redis::RedisWaitConnected) {}

size_t MockClientBase::ShardsCount() const { return 1; }

size_t MockClientBase::ShardByKey(const std::string& /*key*/) const {
  return 0;
}

const std::string& MockClientBase::GetAnyKeyForShard(
    size_t /*shard_idx*/) const {
  static const std::string kKey = "a";
  UASSERT_MSG(ShardsCount() == 1,
              "you should override GetAnyKeyForShard() method if you use it "
              "with ShardsCount() > 1");
  return kKey;
}

std::shared_ptr<Client> MockClientBase::GetClientForShard(size_t shard_idx) {
  return std::make_shared<MockClientBase>(mock_transaction_impl_creator_,
                                          shard_idx);
}

// redis commands:

RequestAppend MockClientBase::Append(
    std::string /*key*/, std::string /*value*/,
    const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestAppend{nullptr};
}

RequestDbsize MockClientBase::Dbsize(
    size_t /*shard*/, const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestDbsize{nullptr};
}

RequestDel MockClientBase::Del(std::string /*key*/,
                               const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestDel{nullptr};
}

RequestDel MockClientBase::Del(std::vector<std::string> /*keys*/,
                               const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestDel{nullptr};
}

RequestUnlink MockClientBase::Unlink(
    std::string /*key*/, const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestUnlink{nullptr};
}

RequestUnlink MockClientBase::Unlink(
    std::vector<std::string> /*keys*/,
    const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestUnlink{nullptr};
}

RequestEvalCommon MockClientBase::EvalCommon(
    std::string /*script*/, std::vector<std::string> /*keys*/,
    std::vector<std::string> /*args*/,
    const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestEvalCommon{nullptr};
}

RequestEvalShaCommon MockClientBase::EvalShaCommon(
    std::string /*script*/, std::vector<std::string> /*keys*/,
    std::vector<std::string> /*args*/,
    const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestEvalShaCommon{nullptr};
}

RequestScriptLoad MockClientBase::ScriptLoad(
    std::string /*script*/, size_t /*shard*/,
    const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestScriptLoad{nullptr};
}

RequestExists MockClientBase::Exists(
    std::string /*key*/, const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestExists{nullptr};
}

RequestExists MockClientBase::Exists(
    std::vector<std::string> /*keys*/,
    const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestExists{nullptr};
}

RequestExpire MockClientBase::Expire(
    std::string /*key*/, std::chrono::seconds /*ttl*/,
    const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestExpire{nullptr};
}

RequestGeoadd MockClientBase::Geoadd(
    std::string /*key*/, GeoaddArg /*point_member*/,
    const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestGeoadd{nullptr};
}

RequestGeoadd MockClientBase::Geoadd(
    std::string /*key*/, std::vector<GeoaddArg> /*point_members*/,
    const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestGeoadd{nullptr};
}

RequestGeoradius MockClientBase::Georadius(
    std::string /*key*/, Longitude /*lon*/, Latitude /*lat*/, double /*radius*/,
    const GeoradiusOptions& /*georadius_options*/,
    const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestGeoradius{nullptr};
}

RequestGeosearch MockClientBase::Geosearch(
    std::string /*key*/, std::string /*member*/, double /*radius*/,
    const GeosearchOptions& /*geosearch_options*/,
    const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestGeosearch{nullptr};
}

RequestGeosearch MockClientBase::Geosearch(
    std::string /*key*/, std::string /*member*/, BoxWidth /*width*/,
    BoxHeight /*height*/, const GeosearchOptions& /*geosearch_options*/,
    const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestGeosearch{nullptr};
}

RequestGeosearch MockClientBase::Geosearch(
    std::string /*key*/, Longitude /*lon*/, Latitude /*lat*/, double /*radius*/,
    const GeosearchOptions& /*geosearch_options*/,
    const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestGeosearch{nullptr};
}

RequestGeosearch MockClientBase::Geosearch(
    std::string /*key*/, Longitude /*lon*/, Latitude /*lat*/,
    BoxWidth /*width*/, BoxHeight /*height*/,
    const GeosearchOptions& /*geosearch_options*/,
    const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestGeosearch{nullptr};
}

RequestGet MockClientBase::Get(std::string /*key*/,
                               const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestGet{nullptr};
}

RequestGetset MockClientBase::Getset(
    std::string /*key*/, std::string /*value*/,
    const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestGetset{nullptr};
}

RequestHdel MockClientBase::Hdel(std::string /*key*/, std::string /*field*/,
                                 const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestHdel{nullptr};
}

RequestHdel MockClientBase::Hdel(std::string /*key*/,
                                 std::vector<std::string> /*fields*/,
                                 const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestHdel{nullptr};
}

RequestHexists MockClientBase::Hexists(
    std::string /*key*/, std::string /*field*/,
    const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestHexists{nullptr};
}

RequestHget MockClientBase::Hget(std::string /*key*/, std::string /*field*/,
                                 const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestHget{nullptr};
}

RequestHgetall MockClientBase::Hgetall(
    std::string /*key*/, const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestHgetall{nullptr};
}

RequestHincrby MockClientBase::Hincrby(
    std::string /*key*/, std::string /*field*/, int64_t /*increment*/,
    const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestHincrby{nullptr};
}

RequestHincrbyfloat MockClientBase::Hincrbyfloat(
    std::string /*key*/, std::string /*field*/, double /*increment*/,
    const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestHincrbyfloat{nullptr};
}

RequestHkeys MockClientBase::Hkeys(std::string /*key*/,
                                   const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestHkeys{nullptr};
}

RequestHlen MockClientBase::Hlen(std::string /*key*/,
                                 const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestHlen{nullptr};
}

RequestHmget MockClientBase::Hmget(std::string /*key*/,
                                   std::vector<std::string> /*fields*/,
                                   const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestHmget{nullptr};
}

RequestHmset MockClientBase::Hmset(
    std::string /*key*/,
    std::vector<std::pair<std::string, std::string>> /*field_values*/,
    const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestHmset{nullptr};
}

ScanRequest<ScanTag::kHscan> MockClientBase::Hscan(
    std::string /*key*/, ScanOptionsTmpl<ScanTag::kHscan> /*options*/,
    const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return ScanRequest<ScanTag::kHscan>{nullptr};
}

RequestHset MockClientBase::Hset(std::string /*key*/, std::string /*field*/,
                                 std::string /*value*/,
                                 const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestHset{nullptr};
}

RequestHsetnx MockClientBase::Hsetnx(
    std::string /*key*/, std::string /*field*/, std::string /*value*/,
    const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestHsetnx{nullptr};
}

RequestHvals MockClientBase::Hvals(std::string /*key*/,
                                   const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestHvals{nullptr};
}

RequestIncr MockClientBase::Incr(std::string /*key*/,
                                 const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestIncr{nullptr};
}

RequestKeys MockClientBase::Keys(std::string /*keys_pattern*/, size_t /*shard*/,
                                 const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestKeys{nullptr};
}

RequestLindex MockClientBase::Lindex(
    std::string /*key*/, int64_t /*index*/,
    const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestLindex{nullptr};
}

RequestLlen MockClientBase::Llen(std::string /*key*/,
                                 const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestLlen{nullptr};
}

RequestLpop MockClientBase::Lpop(std::string /*key*/,
                                 const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestLpop{nullptr};
}

RequestLpush MockClientBase::Lpush(std::string /*key*/, std::string /*value*/,
                                   const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestLpush{nullptr};
}

RequestLpush MockClientBase::Lpush(std::string /*key*/,
                                   std::vector<std::string> /*values*/,
                                   const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestLpush{nullptr};
}

RequestLpushx MockClientBase::Lpushx(
    std::string /*key*/, std::string /*element*/,
    const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestLpushx{nullptr};
}

RequestLrange MockClientBase::Lrange(
    std::string /*key*/, int64_t /*start*/, int64_t /*stop*/,
    const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestLrange{nullptr};
}

RequestLrem MockClientBase::Lrem(std::string /*key*/, int64_t /*count*/,
                                 std::string /*element*/,
                                 const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestLrem{nullptr};
}

RequestLtrim MockClientBase::Ltrim(std::string /*key*/, int64_t /*start*/,
                                   int64_t /*stop*/,
                                   const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestLtrim{nullptr};
}

RequestMget MockClientBase::Mget(std::vector<std::string> /*keys*/,
                                 const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestMget{nullptr};
}

RequestMset MockClientBase::Mset(
    std::vector<std::pair<std::string, std::string>> /*key_values*/,
    const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestMset{nullptr};
}

RequestPersist MockClientBase::Persist(
    std::string /*key*/, const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestPersist{nullptr};
}

RequestPexpire MockClientBase::Pexpire(
    std::string /*key*/, std::chrono::milliseconds /*ttl*/,
    const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestPexpire{nullptr};
}

RequestPing MockClientBase::Ping(size_t /*shard*/,
                                 const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestPing{nullptr};
}

RequestPingMessage MockClientBase::Ping(
    size_t /*shard*/, std::string /*message*/,
    const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestPingMessage{nullptr};
}

void MockClientBase::Publish(std::string /*channel*/, std::string /*message*/,
                             const CommandControl& /*command_control*/,
                             PubShard /*policy*/) {}

RequestRename MockClientBase::Rename(
    std::string /*key*/, std::string /*new_key*/,
    const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestRename{nullptr};
}

RequestRpop MockClientBase::Rpop(std::string /*key*/,
                                 const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestRpop{nullptr};
}

RequestRpush MockClientBase::Rpush(std::string /*key*/, std::string /*value*/,
                                   const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestRpush{nullptr};
}

RequestRpush MockClientBase::Rpush(std::string /*key*/,
                                   std::vector<std::string> /*values*/,
                                   const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestRpush{nullptr};
}

RequestRpushx MockClientBase::Rpushx(
    std::string /*key*/, std::string /*element*/,
    const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestRpushx{nullptr};
}

RequestSadd MockClientBase::Sadd(std::string /*key*/, std::string /*member*/,
                                 const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestSadd{nullptr};
}

RequestSadd MockClientBase::Sadd(std::string /*key*/,
                                 std::vector<std::string> /*members*/,
                                 const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestSadd{nullptr};
}

ScanRequest<ScanTag::kScan> MockClientBase::Scan(
    size_t /*shard*/, ScanOptionsTmpl<ScanTag::kScan> /*options*/,
    const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return ScanRequest<ScanTag::kScan>{nullptr};
}

RequestScard MockClientBase::Scard(std::string /*key*/,
                                   const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestScard{nullptr};
}

RequestSet MockClientBase::Set(std::string /*key*/, std::string /*value*/,
                               const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestSet{nullptr};
}

RequestSet MockClientBase::Set(std::string /*key*/, std::string /*value*/,
                               std::chrono::milliseconds /*ttl*/,
                               const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestSet{nullptr};
}

RequestSetIfExist MockClientBase::SetIfExist(
    std::string /*key*/, std::string /*value*/,
    const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestSetIfExist{nullptr};
}

RequestSetIfExist MockClientBase::SetIfExist(
    std::string /*key*/, std::string /*value*/,
    std::chrono::milliseconds /*ttl*/,
    const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestSetIfExist{nullptr};
}

RequestSetIfNotExist MockClientBase::SetIfNotExist(
    std::string /*key*/, std::string /*value*/,
    const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestSetIfNotExist{nullptr};
}

RequestSetIfNotExist MockClientBase::SetIfNotExist(
    std::string /*key*/, std::string /*value*/,
    std::chrono::milliseconds /*ttl*/,
    const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestSetIfNotExist{nullptr};
}

RequestSetex MockClientBase::Setex(std::string /*key*/,
                                   std::chrono::seconds /*seconds*/,
                                   std::string /*value*/,
                                   const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestSetex{nullptr};
}

RequestSismember MockClientBase::Sismember(
    std::string /*key*/, std::string /*member*/,
    const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestSismember{nullptr};
}

RequestSmembers MockClientBase::Smembers(
    std::string /*key*/, const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestSmembers{nullptr};
}

RequestSrandmember MockClientBase::Srandmember(
    std::string /*key*/, const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestSrandmember{nullptr};
}

RequestSrandmembers MockClientBase::Srandmembers(
    std::string /*key*/, int64_t /*count*/,
    const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestSrandmembers{nullptr};
}

RequestSrem MockClientBase::Srem(std::string /*key*/, std::string /*member*/,
                                 const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestSrem{nullptr};
}

RequestSrem MockClientBase::Srem(std::string /*key*/,
                                 std::vector<std::string> /*members*/,
                                 const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestSrem{nullptr};
}

ScanRequest<ScanTag::kSscan> MockClientBase::Sscan(
    std::string /*key*/, ScanOptionsTmpl<ScanTag::kSscan> /*options*/,
    const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return ScanRequest<ScanTag::kSscan>{nullptr};
}

RequestStrlen MockClientBase::Strlen(
    std::string /*key*/, const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestStrlen{nullptr};
}

RequestTime MockClientBase::Time(size_t /*shard*/,
                                 const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestTime{nullptr};
}

RequestTtl MockClientBase::Ttl(std::string /*key*/,
                               const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestTtl{nullptr};
}

RequestType MockClientBase::Type(std::string /*key*/,
                                 const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestType{nullptr};
}

RequestZadd MockClientBase::Zadd(std::string /*key*/, double /*score*/,
                                 std::string /*member*/,
                                 const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestZadd{nullptr};
}

RequestZadd MockClientBase::Zadd(std::string /*key*/, double /*score*/,
                                 std::string /*member*/,
                                 const ZaddOptions& /*options*/,
                                 const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestZadd{nullptr};
}

RequestZadd MockClientBase::Zadd(
    std::string /*key*/,
    std::vector<std::pair<double, std::string>> /*scored_members*/,
    const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestZadd{nullptr};
}

RequestZadd MockClientBase::Zadd(
    std::string /*key*/,
    std::vector<std::pair<double, std::string>> /*scored_members*/,
    const ZaddOptions& /*options*/, const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestZadd{nullptr};
}

RequestZaddIncr MockClientBase::ZaddIncr(
    std::string /*key*/, double /*score*/, std::string /*member*/,
    const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestZaddIncr{nullptr};
}

RequestZaddIncrExisting MockClientBase::ZaddIncrExisting(
    std::string /*key*/, double /*score*/, std::string /*member*/,
    const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestZaddIncrExisting{nullptr};
}

RequestZcard MockClientBase::Zcard(std::string /*key*/,
                                   const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestZcard{nullptr};
}

RequestZrange MockClientBase::Zrange(
    std::string /*key*/, int64_t /*start*/, int64_t /*stop*/,
    const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestZrange{nullptr};
}

RequestZrangeWithScores MockClientBase::ZrangeWithScores(
    std::string /*key*/, int64_t /*start*/, int64_t /*stop*/,
    const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestZrangeWithScores{nullptr};
}

RequestZrangebyscore MockClientBase::Zrangebyscore(
    std::string /*key*/, double /*min*/, double /*max*/,
    const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestZrangebyscore{nullptr};
}

RequestZrangebyscore MockClientBase::Zrangebyscore(
    std::string /*key*/, std::string /*min*/, std::string /*max*/,
    const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestZrangebyscore{nullptr};
}

RequestZrangebyscore MockClientBase::Zrangebyscore(
    std::string /*key*/, double /*min*/, double /*max*/,
    const RangeOptions& /*range_options*/,
    const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestZrangebyscore{nullptr};
}

RequestZrangebyscore MockClientBase::Zrangebyscore(
    std::string /*key*/, std::string /*min*/, std::string /*max*/,
    const RangeOptions& /*range_options*/,
    const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestZrangebyscore{nullptr};
}

RequestZrangebyscoreWithScores MockClientBase::ZrangebyscoreWithScores(
    std::string /*key*/, double /*min*/, double /*max*/,
    const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestZrangebyscoreWithScores{nullptr};
}

RequestZrangebyscoreWithScores MockClientBase::ZrangebyscoreWithScores(
    std::string /*key*/, std::string /*min*/, std::string /*max*/,
    const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestZrangebyscoreWithScores{nullptr};
}

RequestZrangebyscoreWithScores MockClientBase::ZrangebyscoreWithScores(
    std::string /*key*/, double /*min*/, double /*max*/,
    const RangeOptions& /*range_options*/,
    const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestZrangebyscoreWithScores{nullptr};
}

RequestZrangebyscoreWithScores MockClientBase::ZrangebyscoreWithScores(
    std::string /*key*/, std::string /*min*/, std::string /*max*/,
    const RangeOptions& /*range_options*/,
    const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestZrangebyscoreWithScores{nullptr};
}

RequestZrem MockClientBase::Zrem(std::string /*key*/, std::string /*member*/,
                                 const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestZrem{nullptr};
}

RequestZrem MockClientBase::Zrem(std::string /*key*/,
                                 std::vector<std::string> /*members*/,
                                 const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestZrem{nullptr};
}

RequestZremrangebyrank MockClientBase::Zremrangebyrank(
    std::string /*key*/, int64_t /*start*/, int64_t /*stop*/,
    const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestZremrangebyrank{nullptr};
}

RequestZremrangebyscore MockClientBase::Zremrangebyscore(
    std::string /*key*/, double /*min*/, double /*max*/,
    const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestZremrangebyscore{nullptr};
}

RequestZremrangebyscore MockClientBase::Zremrangebyscore(
    std::string /*key*/, std::string /*min*/, std::string /*max*/,
    const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestZremrangebyscore{nullptr};
}

ScanRequest<ScanTag::kZscan> MockClientBase::Zscan(
    std::string /*key*/, ScanOptionsTmpl<ScanTag::kZscan> /*options*/,
    const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return ScanRequest<ScanTag::kZscan>{nullptr};
}

RequestZscore MockClientBase::Zscore(
    std::string /*key*/, std::string /*member*/,
    const CommandControl& /*command_control*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestZscore{nullptr};
}

// end of redis commands

TransactionPtr MockClientBase::Multi() {
  UASSERT_MSG(!!mock_transaction_impl_creator_,
              "MockTransactionImpl type not set");
  return std::make_unique<MockTransaction>(shared_from_this(),
                                           (*mock_transaction_impl_creator_)());
}

TransactionPtr MockClientBase::Multi(Transaction::CheckShards check_shards) {
  UASSERT_MSG(!!mock_transaction_impl_creator_,
              "MockTransactionImpl type not set");
  return std::make_unique<MockTransaction>(
      shared_from_this(), (*mock_transaction_impl_creator_)(), check_shards);
}

}  // namespace storages::redis

USERVER_NAMESPACE_END
