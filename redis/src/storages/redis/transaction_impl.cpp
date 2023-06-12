#include <storages/redis/transaction_impl.hpp>

#include <sstream>

#include <userver/storages/redis/impl/transaction_subrequest_data.hpp>

#include "client_impl.hpp"
#include "request_exec_data_impl.hpp"

USERVER_NAMESPACE_BEGIN

namespace storages::redis {
namespace {

RequestExec CreateExecRequest(
    USERVER_NAMESPACE::redis::Request&& request,
    std::vector<TransactionImpl::ResultPromise>&& result_promises) {
  return RequestExec(std::make_unique<RequestExecDataImpl>(
      std::move(request), std::move(result_promises)));
}

}  // namespace

TransactionImpl::TransactionImpl(std::shared_ptr<ClientImpl> client,
                                 CheckShards check_shards)
    : client_(std::move(client)),
      check_shards_(check_shards),
      cmd_args_({"MULTI"}) {}

RequestExec TransactionImpl::Exec(const CommandControl& command_control) {
  if (!shard_) {
    throw EmptyTransactionException(
        "Can't determine shard. Empty transaction?");
  }
  auto client_force_shard_idx = client_->GetForcedShardIdx();
  if (client_force_shard_idx) {
    if (command_control.force_shard_idx &&
        *command_control.force_shard_idx != *client_force_shard_idx)
      throw USERVER_NAMESPACE::redis::InvalidArgumentException(
          "forced shard idx from CommandControl != forced shard for client (" +
          std::to_string(*command_control.force_shard_idx) +
          " != " + std::to_string(*client_force_shard_idx) + ')');
    shard_ = *client_force_shard_idx;
  } else if (command_control.force_shard_idx) {
    shard_ = *command_control.force_shard_idx;
  }
  client_->CheckShardIdx(*shard_);
  cmd_args_.Then("EXEC");
  auto replies_to_skip = result_promises_.size() + 1;
  const auto master = master_;
  master_ = false;
  return CreateExecRequest(
      client_->MakeRequest(std::move(cmd_args_), *shard_, master,
                           client_->GetCommandControl(command_control),
                           replies_to_skip),
      std::move(result_promises_));
}

// redis commands:

RequestAppend TransactionImpl::Append(std::string key, std::string value) {
  UpdateShard(key);
  return AddCmd<RequestAppend>("append", true, std::move(key),
                               std::move(value));
}

RequestDbsize TransactionImpl::Dbsize(size_t shard) {
  UpdateShard(shard);
  return AddCmd<RequestDbsize>("dbsize", false);
}

RequestDel TransactionImpl::Del(std::string key) {
  UpdateShard(key);
  return AddCmd<RequestDel>("del", true, std::move(key));
}

RequestDel TransactionImpl::Del(std::vector<std::string> keys) {
  UpdateShard(keys);
  return AddCmd<RequestDel>("del", true, std::move(keys));
}

RequestUnlink TransactionImpl::Unlink(std::string key) {
  UpdateShard(key);
  return AddCmd<RequestUnlink>("unlink", true, std::move(key));
}

RequestUnlink TransactionImpl::Unlink(std::vector<std::string> keys) {
  UpdateShard(keys);
  return AddCmd<RequestUnlink>("unlink", true, std::move(keys));
}

RequestExists TransactionImpl::Exists(std::string key) {
  UpdateShard(key);
  return AddCmd<RequestExists>("exists", false, std::move(key));
}

RequestExists TransactionImpl::Exists(std::vector<std::string> keys) {
  UpdateShard(keys);
  return AddCmd<RequestExists>("exists", false, std::move(keys));
}

RequestExpire TransactionImpl::Expire(std::string key,
                                      std::chrono::seconds ttl) {
  UpdateShard(key);
  return AddCmd<RequestExpire>("expire", true, std::move(key), ttl.count());
}

RequestGeoadd TransactionImpl::Geoadd(std::string key, GeoaddArg point_member) {
  UpdateShard(key);
  return AddCmd<RequestGeoadd>("geoadd", true, std::move(key),
                               std::move(point_member));
}

RequestGeoadd TransactionImpl::Geoadd(std::string key,
                                      std::vector<GeoaddArg> point_members) {
  UpdateShard(key);
  return AddCmd<RequestGeoadd>("geoadd", true, std::move(key),
                               std::move(point_members));
}

RequestGeoradius TransactionImpl::Georadius(
    std::string key, Longitude lon, Latitude lat, double radius,
    const GeoradiusOptions& georadius_options) {
  UpdateShard(key);
  return AddCmd<RequestGeoradius>("georadius_ro", false, std::move(key),
                                  lon.GetUnderlying(), lat.GetUnderlying(),
                                  radius, georadius_options);
}

RequestGeosearch TransactionImpl::Geosearch(
    std::string key, std::string member, double radius,
    const GeosearchOptions& geosearch_options) {
  UpdateShard(key);
  return AddCmd<RequestGeosearch>("geosearch", false, std::move(key),
                                  "FROMMEMBER", std::move(member), "BYRADIUS",
                                  radius, geosearch_options);
}

RequestGeosearch TransactionImpl::Geosearch(
    std::string key, std::string member, BoxWidth width, BoxHeight height,
    const GeosearchOptions& geosearch_options) {
  UpdateShard(key);
  return AddCmd<RequestGeosearch>("geosearch", false, std::move(key),
                                  "FROMMEMBER", std::move(member), "BYBOX",
                                  width.GetUnderlying(), height.GetUnderlying(),
                                  geosearch_options);
}

RequestGeosearch TransactionImpl::Geosearch(
    std::string key, Longitude lon, Latitude lat, double radius,
    const GeosearchOptions& geosearch_options) {
  UpdateShard(key);
  return AddCmd<RequestGeosearch>(
      "geosearch", false, std::move(key), "FROMLONLAT", lon.GetUnderlying(),
      lat.GetUnderlying(), "BYRADIUS", radius, geosearch_options);
}

RequestGeosearch TransactionImpl::Geosearch(
    std::string key, Longitude lon, Latitude lat, BoxWidth width,
    BoxHeight height, const GeosearchOptions& geosearch_options) {
  UpdateShard(key);
  return AddCmd<RequestGeosearch>(
      "geosearch", false, std::move(key), "FROMLONLAT", lon.GetUnderlying(),
      lat.GetUnderlying(), "BYBOX", width.GetUnderlying(),
      height.GetUnderlying(), geosearch_options);
}

RequestGet TransactionImpl::Get(std::string key) {
  UpdateShard(key);
  return AddCmd<RequestGet>("get", false, std::move(key));
}

RequestGetset TransactionImpl::Getset(std::string key, std::string value) {
  UpdateShard(key);
  return AddCmd<RequestGetset>("getset", true, std::move(key),
                               std::move(value));
}

RequestHdel TransactionImpl::Hdel(std::string key, std::string field) {
  UpdateShard(key);
  return AddCmd<RequestHdel>("hdel", true, std::move(key), std::move(field));
}

RequestHdel TransactionImpl::Hdel(std::string key,
                                  std::vector<std::string> fields) {
  UpdateShard(key);
  return AddCmd<RequestHdel>("hdel", true, std::move(key), std::move(fields));
}

RequestHexists TransactionImpl::Hexists(std::string key, std::string field) {
  UpdateShard(key);
  return AddCmd<RequestHexists>("hexists", false, std::move(key),
                                std::move(field));
}

RequestHget TransactionImpl::Hget(std::string key, std::string field) {
  UpdateShard(key);
  return AddCmd<RequestHget>("hget", false, std::move(key), std::move(field));
}

RequestHgetall TransactionImpl::Hgetall(std::string key) {
  UpdateShard(key);
  return AddCmd<RequestHgetall>("hgetall", false, std::move(key));
}

RequestHincrby TransactionImpl::Hincrby(std::string key, std::string field,
                                        int64_t increment) {
  UpdateShard(key);
  return AddCmd<RequestHincrby>("hincrby", true, std::move(key),
                                std::move(field), increment);
}

RequestHincrbyfloat TransactionImpl::Hincrbyfloat(std::string key,
                                                  std::string field,
                                                  double increment) {
  UpdateShard(key);
  return AddCmd<RequestHincrbyfloat>("hincrbyfloat", true, std::move(key),
                                     std::move(field), increment);
}

RequestHkeys TransactionImpl::Hkeys(std::string key) {
  UpdateShard(key);
  return AddCmd<RequestHkeys>("hkeys", false, std::move(key));
}

RequestHlen TransactionImpl::Hlen(std::string key) {
  UpdateShard(key);
  return AddCmd<RequestHlen>("hlen", false, std::move(key));
}

RequestHmget TransactionImpl::Hmget(std::string key,
                                    std::vector<std::string> fields) {
  UpdateShard(key);
  return AddCmd<RequestHmget>("hmget", false, std::move(key),
                              std::move(fields));
}

RequestHmset TransactionImpl::Hmset(
    std::string key,
    std::vector<std::pair<std::string, std::string>> field_values) {
  UpdateShard(key);
  return AddCmd<RequestHmset>("hmset", true, std::move(key),
                              std::move(field_values));
}

RequestHset TransactionImpl::Hset(std::string key, std::string field,
                                  std::string value) {
  UpdateShard(key);
  return AddCmd<RequestHset>("hset", true, std::move(key), std::move(field),
                             std::move(value));
}

RequestHsetnx TransactionImpl::Hsetnx(std::string key, std::string field,
                                      std::string value) {
  UpdateShard(key);
  return AddCmd<RequestHsetnx>("hsetnx", true, std::move(key), std::move(field),
                               std::move(value));
}

RequestHvals TransactionImpl::Hvals(std::string key) {
  UpdateShard(key);
  return AddCmd<RequestHvals>("hvals", false, std::move(key));
}

RequestIncr TransactionImpl::Incr(std::string key) {
  UpdateShard(key);
  return AddCmd<RequestIncr>("incr", true, std::move(key));
}

RequestKeys TransactionImpl::Keys(std::string keys_pattern, size_t shard) {
  UpdateShard(shard);
  return AddCmd<RequestKeys>("keys", false, std::move(keys_pattern));
}

RequestLindex TransactionImpl::Lindex(std::string key, int64_t index) {
  UpdateShard(key);
  return AddCmd<RequestLindex>("lindex", false, std::move(key), index);
}

RequestLlen TransactionImpl::Llen(std::string key) {
  UpdateShard(key);
  return AddCmd<RequestLlen>("llen", false, std::move(key));
}

RequestLpop TransactionImpl::Lpop(std::string key) {
  UpdateShard(key);
  return AddCmd<RequestLpop>("lpop", true, std::move(key));
}

RequestLpush TransactionImpl::Lpush(std::string key, std::string value) {
  UpdateShard(key);
  return AddCmd<RequestLpush>("lpush", true, std::move(key), std::move(value));
}

RequestLpush TransactionImpl::Lpush(std::string key,
                                    std::vector<std::string> values) {
  UpdateShard(key);
  return AddCmd<RequestLpush>("lpush", true, std::move(key), std::move(values));
}

RequestLpushx TransactionImpl::Lpushx(std::string key, std::string element) {
  UpdateShard(key);
  return AddCmd<RequestLpushx>("lpushx", true, std::move(key),
                               std::move(element));
}

RequestLrange TransactionImpl::Lrange(std::string key, int64_t start,
                                      int64_t stop) {
  UpdateShard(key);
  return AddCmd<RequestLrange>("lrange", false, std::move(key), start, stop);
}

RequestLrem TransactionImpl::Lrem(std::string key, int64_t count,
                                  std::string element) {
  UpdateShard(key);
  return AddCmd<RequestLrem>("lrem", true, std::move(key), count,
                             std::move(element));
}

RequestLtrim TransactionImpl::Ltrim(std::string key, int64_t start,
                                    int64_t stop) {
  UpdateShard(key);
  return AddCmd<RequestLtrim>("ltrim", true, std::move(key), start, stop);
}

RequestMget TransactionImpl::Mget(std::vector<std::string> keys) {
  UpdateShard(keys);
  return AddCmd<RequestMget>("mget", false, std::move(keys));
}

RequestMset TransactionImpl::Mset(
    std::vector<std::pair<std::string, std::string>> key_values) {
  UpdateShard(key_values);
  return AddCmd<RequestMset>("mset", true, std::move(key_values));
}

RequestPersist TransactionImpl::Persist(std::string key) {
  UpdateShard(key);
  return AddCmd<RequestPersist>("persist", true, std::move(key));
}

RequestPexpire TransactionImpl::Pexpire(std::string key,
                                        std::chrono::milliseconds ttl) {
  UpdateShard(key);
  return AddCmd<RequestPexpire>("pexpire", true, std::move(key), ttl.count());
}

RequestPing TransactionImpl::Ping(size_t shard) {
  UpdateShard(shard);
  return AddCmd<RequestPing>("ping", false);
}

RequestPingMessage TransactionImpl::PingMessage(size_t shard,
                                                std::string message) {
  UpdateShard(shard);
  return AddCmd<RequestPingMessage>("ping", false, std::move(message));
}

RequestRename TransactionImpl::Rename(std::string key, std::string new_key) {
  UpdateShard(key);
  UpdateShard(new_key);
  return AddCmd<RequestRename>("rename", true, std::move(key),
                               std::move(new_key));
}

RequestRpop TransactionImpl::Rpop(std::string key) {
  UpdateShard(key);
  return AddCmd<RequestRpop>("rpop", true, std::move(key));
}

RequestRpush TransactionImpl::Rpush(std::string key, std::string value) {
  UpdateShard(key);
  return AddCmd<RequestRpush>("rpush", true, std::move(key), std::move(value));
}

RequestRpush TransactionImpl::Rpush(std::string key,
                                    std::vector<std::string> values) {
  UpdateShard(key);
  return AddCmd<RequestRpush>("rpush", true, std::move(key), std::move(values));
}

RequestRpushx TransactionImpl::Rpushx(std::string key, std::string element) {
  UpdateShard(key);
  return AddCmd<RequestRpushx>("rpushx", true, std::move(key),
                               std::move(element));
}

RequestSadd TransactionImpl::Sadd(std::string key, std::string member) {
  UpdateShard(key);
  return AddCmd<RequestSadd>("sadd", true, std::move(key), std::move(member));
}

RequestSadd TransactionImpl::Sadd(std::string key,
                                  std::vector<std::string> members) {
  UpdateShard(key);
  return AddCmd<RequestSadd>("sadd", true, std::move(key), std::move(members));
}

RequestScard TransactionImpl::Scard(std::string key) {
  UpdateShard(key);
  return AddCmd<RequestScard>("scard", false, std::move(key));
}

RequestSet TransactionImpl::Set(std::string key, std::string value) {
  UpdateShard(key);
  return AddCmd<RequestSet>("set", true, std::move(key), std::move(value));
}

RequestSet TransactionImpl::Set(std::string key, std::string value,
                                std::chrono::milliseconds ttl) {
  UpdateShard(key);
  return AddCmd<RequestSet>("set", true, std::move(key), std::move(value), "PX",
                            ttl.count());
}

RequestSetIfExist TransactionImpl::SetIfExist(std::string key,
                                              std::string value) {
  UpdateShard(key);
  return AddCmd<RequestSetIfExist>("set", true, std::move(key),
                                   std::move(value), "XX");
}

RequestSetIfExist TransactionImpl::SetIfExist(std::string key,
                                              std::string value,
                                              std::chrono::milliseconds ttl) {
  UpdateShard(key);
  return AddCmd<RequestSetIfExist>("set", true, std::move(key),
                                   std::move(value), "PX", ttl.count(), "XX");
}

RequestSetIfNotExist TransactionImpl::SetIfNotExist(std::string key,
                                                    std::string value) {
  UpdateShard(key);
  return AddCmd<RequestSetIfNotExist>("set", true, std::move(key),
                                      std::move(value), "NX");
}

RequestSetIfNotExist TransactionImpl::SetIfNotExist(
    std::string key, std::string value, std::chrono::milliseconds ttl) {
  UpdateShard(key);
  return AddCmd<RequestSetIfNotExist>(
      "set", true, std::move(key), std::move(value), "PX", ttl.count(), "NX");
}

RequestSetex TransactionImpl::Setex(std::string key,
                                    std::chrono::seconds seconds,
                                    std::string value) {
  UpdateShard(key);
  return AddCmd<RequestSetex>("setex", true, std::move(key), seconds.count(),
                              std::move(value));
}

RequestSismember TransactionImpl::Sismember(std::string key,
                                            std::string member) {
  UpdateShard(key);
  return AddCmd<RequestSismember>("sismember", false, std::move(key),
                                  std::move(member));
}

RequestSmembers TransactionImpl::Smembers(std::string key) {
  UpdateShard(key);
  return AddCmd<RequestSmembers>("smembers", false, std::move(key));
}

RequestSrandmember TransactionImpl::Srandmember(std::string key) {
  UpdateShard(key);
  return AddCmd<RequestSrandmember>("srandmember", false, std::move(key));
}

RequestSrandmembers TransactionImpl::Srandmembers(std::string key,
                                                  int64_t count) {
  UpdateShard(key);
  return AddCmd<RequestSrandmembers>("srandmember", false, std::move(key),
                                     count);
}

RequestSrem TransactionImpl::Srem(std::string key, std::string member) {
  UpdateShard(key);
  return AddCmd<RequestSrem>("srem", true, std::move(key), std::move(member));
}

RequestSrem TransactionImpl::Srem(std::string key,
                                  std::vector<std::string> members) {
  UpdateShard(key);
  return AddCmd<RequestSrem>("srem", true, std::move(key), std::move(members));
}

RequestStrlen TransactionImpl::Strlen(std::string key) {
  UpdateShard(key);
  return AddCmd<RequestStrlen>("strlen", false, std::move(key));
}

RequestTime TransactionImpl::Time(size_t shard) {
  UpdateShard(shard);
  return AddCmd<RequestTime>("time", false);
}

RequestTtl TransactionImpl::Ttl(std::string key) {
  UpdateShard(key);
  return AddCmd<RequestTtl>("ttl", false, std::move(key));
}

RequestType TransactionImpl::Type(std::string key) {
  UpdateShard(key);
  return AddCmd<RequestType>("type", false, std::move(key));
}

RequestZadd TransactionImpl::Zadd(std::string key, double score,
                                  std::string member) {
  UpdateShard(key);
  return AddCmd<RequestZadd>("zadd", true, std::move(key), score,
                             std::move(member));
}

RequestZadd TransactionImpl::Zadd(std::string key, double score,
                                  std::string member,
                                  const ZaddOptions& options) {
  UpdateShard(key);
  return AddCmd<RequestZadd>("zadd", true, std::move(key), options, score,
                             std::move(member));
}

RequestZadd TransactionImpl::Zadd(
    std::string key,
    std::vector<std::pair<double, std::string>> scored_members) {
  UpdateShard(key);
  return AddCmd<RequestZadd>("zadd", true, std::move(key),
                             std::move(scored_members));
}

RequestZadd TransactionImpl::Zadd(
    std::string key, std::vector<std::pair<double, std::string>> scored_members,
    const ZaddOptions& options) {
  UpdateShard(key);
  return AddCmd<RequestZadd>("zadd", true, std::move(key), options,
                             std::move(scored_members));
}

RequestZaddIncr TransactionImpl::ZaddIncr(std::string key, double score,
                                          std::string member) {
  UpdateShard(key);
  return AddCmd<RequestZaddIncr>("zadd", true, std::move(key), "INCR", score,
                                 std::move(member));
}

RequestZaddIncrExisting TransactionImpl::ZaddIncrExisting(std::string key,
                                                          double score,
                                                          std::string member) {
  UpdateShard(key);
  return AddCmd<RequestZaddIncrExisting>("zadd", true, std::move(key), "XX",
                                         "INCR", score, std::move(member));
}

RequestZcard TransactionImpl::Zcard(std::string key) {
  UpdateShard(key);
  return AddCmd<RequestZcard>("zcard", false, std::move(key));
}

RequestZrange TransactionImpl::Zrange(std::string key, int64_t start,
                                      int64_t stop) {
  UpdateShard(key);
  return AddCmd<RequestZrange>("zrange", false, std::move(key), start, stop);
}

RequestZrangeWithScores TransactionImpl::ZrangeWithScores(std::string key,
                                                          int64_t start,
                                                          int64_t stop) {
  UpdateShard(key);
  USERVER_NAMESPACE::redis::ScoreOptions with_scores{true};
  return AddCmd<RequestZrangeWithScores>("zrange", false, std::move(key), start,
                                         stop, with_scores);
}

RequestZrangebyscore TransactionImpl::Zrangebyscore(std::string key, double min,
                                                    double max) {
  UpdateShard(key);
  return AddCmd<RequestZrangebyscore>("zrangebyscore", false, std::move(key),
                                      min, max);
}

RequestZrangebyscore TransactionImpl::Zrangebyscore(std::string key,
                                                    std::string min,
                                                    std::string max) {
  UpdateShard(key);
  return AddCmd<RequestZrangebyscore>("zrangebyscore", false, std::move(key),
                                      std::move(min), std::move(max));
}

RequestZrangebyscore TransactionImpl::Zrangebyscore(
    std::string key, double min, double max,
    const RangeOptions& range_options) {
  UpdateShard(key);
  return AddCmd<RequestZrangebyscore>("zrangebyscore", false, std::move(key),
                                      min, max, range_options);
}

RequestZrangebyscore TransactionImpl::Zrangebyscore(
    std::string key, std::string min, std::string max,
    const RangeOptions& range_options) {
  UpdateShard(key);
  return AddCmd<RequestZrangebyscore>("zrangebyscore", false, std::move(key),
                                      std::move(min), std::move(max),
                                      range_options);
}

RequestZrangebyscoreWithScores TransactionImpl::ZrangebyscoreWithScores(
    std::string key, double min, double max) {
  UpdateShard(key);
  USERVER_NAMESPACE::redis::RangeScoreOptions range_score_options{{true}, {}};
  return AddCmd<RequestZrangebyscoreWithScores>(
      "zrangebyscore", false, std::move(key), min, max, range_score_options);
}

RequestZrangebyscoreWithScores TransactionImpl::ZrangebyscoreWithScores(
    std::string key, std::string min, std::string max) {
  UpdateShard(key);
  USERVER_NAMESPACE::redis::RangeScoreOptions range_score_options{{true}, {}};
  return AddCmd<RequestZrangebyscoreWithScores>(
      "zrangebyscore", false, std::move(key), std::move(min), std::move(max),
      range_score_options);
}

RequestZrangebyscoreWithScores TransactionImpl::ZrangebyscoreWithScores(
    std::string key, double min, double max,
    const RangeOptions& range_options) {
  UpdateShard(key);
  USERVER_NAMESPACE::redis::RangeScoreOptions range_score_options{
      {true}, range_options};
  return AddCmd<RequestZrangebyscoreWithScores>(
      "zrangebyscore", false, std::move(key), min, max, range_score_options);
}

RequestZrangebyscoreWithScores TransactionImpl::ZrangebyscoreWithScores(
    std::string key, std::string min, std::string max,
    const RangeOptions& range_options) {
  UpdateShard(key);
  USERVER_NAMESPACE::redis::RangeScoreOptions range_score_options{
      {true}, range_options};
  return AddCmd<RequestZrangebyscoreWithScores>(
      "zrangebyscore", false, std::move(key), std::move(min), std::move(max),
      range_score_options);
}

RequestZrem TransactionImpl::Zrem(std::string key, std::string member) {
  UpdateShard(key);
  return AddCmd<RequestZrem>("zrem", true, std::move(key), std::move(member));
}

RequestZrem TransactionImpl::Zrem(std::string key,
                                  std::vector<std::string> members) {
  UpdateShard(key);
  return AddCmd<RequestZrem>("zrem", true, std::move(key), std::move(members));
}

RequestZremrangebyrank TransactionImpl::Zremrangebyrank(std::string key,
                                                        int64_t start,
                                                        int64_t stop) {
  UpdateShard(key);
  return AddCmd<RequestZremrangebyrank>("zremrangebyrank", true, std::move(key),
                                        start, stop);
}

RequestZremrangebyscore TransactionImpl::Zremrangebyscore(std::string key,
                                                          double min,
                                                          double max) {
  UpdateShard(key);
  return AddCmd<RequestZremrangebyscore>("zremrangebyscore", true,
                                         std::move(key), min, max);
}

RequestZremrangebyscore TransactionImpl::Zremrangebyscore(std::string key,
                                                          std::string min,
                                                          std::string max) {
  UpdateShard(key);
  return AddCmd<RequestZremrangebyscore>(
      "zremrangebyscore", true, std::move(key), std::move(min), std::move(max));
}

RequestZscore TransactionImpl::Zscore(std::string key, std::string member) {
  UpdateShard(key);
  return AddCmd<RequestZscore>("zscore", false, std::move(key),
                               std::move(member));
}

// end of redis commands

void TransactionImpl::UpdateShard(const std::string& key) {
  try {
    UpdateShard(client_->ShardByKey(key));
  } catch (const USERVER_NAMESPACE::redis::InvalidArgumentException& ex) {
    throw USERVER_NAMESPACE::redis::InvalidArgumentException(
        ex.what() + std::string{" for key=" + key});
  }
}

void TransactionImpl::UpdateShard(const std::vector<std::string>& keys) {
  for (const auto& key : keys) {
    UpdateShard(key);
  }
}

void TransactionImpl::UpdateShard(
    const std::vector<std::pair<std::string, std::string>>& key_values) {
  for (const auto& key_value : key_values) {
    UpdateShard(key_value.first);
  }
}

void TransactionImpl::UpdateShard(size_t shard) {
  if (shard_) {
    if (check_shards_ == CheckShards::kSame && *shard_ != shard) {
      std::ostringstream os;
      os << "Storages::redis::Transaction must deal with the same shard across "
            "all the operations. Shard="
         << *shard_
         << " was detected by first command, but one of the commands used "
            "shard="
         << shard;
      throw USERVER_NAMESPACE::redis::InvalidArgumentException(os.str());
    }
  } else {
    shard_ = shard;
  }
}

template <typename Result, typename ReplyType>
Request<Result, ReplyType> TransactionImpl::DoAddCmd(
    To<Request<Result, ReplyType>> to) {
  engine::Promise<ReplyType> promise;
  Request<Result, ReplyType> request(
      std::make_unique<impl::TransactionSubrequestDataImpl<ReplyType>>(
          promise.get_future()));
  result_promises_.emplace_back(std::move(promise), to);
  return request;
}

template <typename Request, typename... Args>
Request TransactionImpl::AddCmd(std::string command, bool master,
                                Args&&... args) {
  master_ |= master;
  auto request = DoAddCmd(To<Request>{});
  cmd_args_.Then(std::move(command), std::forward<Args>(args)...);
  return request;
}

}  // namespace storages::redis

USERVER_NAMESPACE_END
