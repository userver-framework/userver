#include <userver/storages/redis/mock_transaction.hpp>

#include <vector>

#include <userver/utils/assert.hpp>

#include <userver/storages/redis/impl/transaction_subrequest_data.hpp>
#include <userver/storages/redis/mock_client_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

class MockTransaction::ResultPromise {
 public:
  template <typename Result, typename ReplyType>
  ResultPromise(engine::Promise<ReplyType>&& promise,
                Request<Result, ReplyType>&& subrequest)
      : impl_(std::make_unique<ResultPromiseImpl<Result, ReplyType>>(
            std::move(promise), std::move(subrequest))) {}

  ResultPromise(ResultPromise&& other) = default;

  void ProcessReply(const std::string& request_description) {
    impl_->ProcessReply(request_description);
  }

 private:
  class ResultPromiseImplBase {
   public:
    virtual ~ResultPromiseImplBase() = default;

    virtual void ProcessReply(const std::string& request_description) = 0;
  };

  template <typename Result, typename ReplyType>
  class ResultPromiseImpl : public ResultPromiseImplBase {
   public:
    ResultPromiseImpl(engine::Promise<ReplyType>&& promise,
                      Request<Result, ReplyType>&& subrequest)
        : promise_(std::move(promise)), subrequest_(std::move(subrequest)) {}

    void ProcessReply(const std::string& request_description) override {
      try {
        if constexpr (std::is_same<ReplyType, void>::value) {
          subrequest_.Get(request_description);
          promise_.set_value();
        } else {
          promise_.set_value(subrequest_.Get(request_description));
        }
      } catch (const USERVER_NAMESPACE::redis::RequestFailedException&) {
        throw;
      } catch (const std::exception&) {
        promise_.set_exception(std::current_exception());
      }
    }

   private:
    engine::Promise<ReplyType> promise_;
    Request<Result, ReplyType> subrequest_;
  };

  std::unique_ptr<ResultPromiseImplBase> impl_;
};

class MockTransaction::MockRequestExecDataImpl final
    : public RequestDataBase<void> {
 public:
  MockRequestExecDataImpl(
      std::vector<std::unique_ptr<ResultPromise>>&& result_promises)
      : result_promises_(std::move(result_promises)) {}

  void Wait() override {}

  void Get(const std::string& request_description) override {
    for (auto& result_promise : result_promises_) {
      result_promise->ProcessReply(request_description);
    }
  }

  ReplyPtr GetRaw() override {
    UASSERT_MSG(false, "not supported in mocked request");
    return nullptr;
  }

 private:
  std::vector<std::unique_ptr<ResultPromise>> result_promises_;
};

MockTransaction::MockTransaction(std::shared_ptr<MockClientBase> client,
                                 std::unique_ptr<MockTransactionImplBase> impl,
                                 CheckShards check_shards)
    : client_(std::move(client)),
      check_shards_(check_shards),
      impl_(std::move(impl)) {}

MockTransaction::~MockTransaction() = default;

RequestExec MockTransaction::Exec(const CommandControl& command_control) {
  if (!shard_) {
    throw EmptyTransactionException(
        "Can't determine shard. Empty transaction?");
  }
  if (command_control.force_shard_idx)
    shard_ = *command_control.force_shard_idx;
  client_->CheckShardIdx(*shard_);
  return CreateMockExecRequest();
}

// redis commands:

RequestAppend MockTransaction::Append(std::string key, std::string value) {
  UpdateShard(key);
  return AddSubrequest(impl_->Append(std::move(key), std::move(value)));
}

RequestDbsize MockTransaction::Dbsize(size_t shard) {
  UpdateShard(shard);
  return AddSubrequest(impl_->Dbsize(shard));
}

RequestDel MockTransaction::Del(std::string key) {
  UpdateShard(key);
  return AddSubrequest(impl_->Del(std::move(key)));
}

RequestDel MockTransaction::Del(std::vector<std::string> keys) {
  UpdateShard(keys);
  return AddSubrequest(impl_->Del(std::move(keys)));
}

RequestUnlink MockTransaction::Unlink(std::string key) {
  UpdateShard(key);
  return AddSubrequest(impl_->Unlink(std::move(key)));
}

RequestUnlink MockTransaction::Unlink(std::vector<std::string> keys) {
  UpdateShard(keys);
  return AddSubrequest(impl_->Unlink(std::move(keys)));
}

RequestExists MockTransaction::Exists(std::string key) {
  UpdateShard(key);
  return AddSubrequest(impl_->Exists(std::move(key)));
}

RequestExists MockTransaction::Exists(std::vector<std::string> keys) {
  UpdateShard(keys);
  return AddSubrequest(impl_->Exists(std::move(keys)));
}

RequestExpire MockTransaction::Expire(std::string key,
                                      std::chrono::seconds ttl) {
  UpdateShard(key);
  return AddSubrequest(impl_->Expire(std::move(key), ttl));
}

RequestGeoadd MockTransaction::Geoadd(std::string key, GeoaddArg point_member) {
  UpdateShard(key);
  return AddSubrequest(impl_->Geoadd(std::move(key), point_member));
}

RequestGeoadd MockTransaction::Geoadd(std::string key,
                                      std::vector<GeoaddArg> point_members) {
  UpdateShard(key);
  return AddSubrequest(impl_->Geoadd(std::move(key), std::move(point_members)));
}

RequestGeoradius MockTransaction::Georadius(
    std::string key, Longitude lon, Latitude lat, double radius,
    const GeoradiusOptions& georadius_options) {
  UpdateShard(key);
  return AddSubrequest(
      impl_->Georadius(std::move(key), lon, lat, radius, georadius_options));
}

RequestGeosearch MockTransaction::Geosearch(
    std::string key, std::string member, double radius,
    const GeosearchOptions& geosearch_options) {
  UpdateShard(key);
  return AddSubrequest(impl_->Geosearch(std::move(key), std::move(member),
                                        radius, geosearch_options));
}

RequestGeosearch MockTransaction::Geosearch(
    std::string key, std::string member, BoxWidth width, BoxHeight height,
    const GeosearchOptions& geosearch_options) {
  UpdateShard(key);
  return AddSubrequest(impl_->Geosearch(std::move(key), std::move(member),
                                        width, height, geosearch_options));
}

RequestGeosearch MockTransaction::Geosearch(
    std::string key, Longitude lon, Latitude lat, double radius,
    const GeosearchOptions& geosearch_options) {
  UpdateShard(key);
  return AddSubrequest(
      impl_->Geosearch(std::move(key), lon, lat, radius, geosearch_options));
}

RequestGeosearch MockTransaction::Geosearch(
    std::string key, Longitude lon, Latitude lat, BoxWidth width,
    BoxHeight height, const GeosearchOptions& geosearch_options) {
  UpdateShard(key);
  return AddSubrequest(impl_->Geosearch(std::move(key), lon, lat, width, height,
                                        geosearch_options));
}

RequestGet MockTransaction::Get(std::string key) {
  UpdateShard(key);
  return AddSubrequest(impl_->Get(std::move(key)));
}

RequestGetset MockTransaction::Getset(std::string key, std::string value) {
  UpdateShard(key);
  return AddSubrequest(impl_->Getset(std::move(key), std::move(value)));
}

RequestHdel MockTransaction::Hdel(std::string key, std::string field) {
  UpdateShard(key);
  return AddSubrequest(impl_->Hdel(std::move(key), std::move(field)));
}

RequestHdel MockTransaction::Hdel(std::string key,
                                  std::vector<std::string> fields) {
  UpdateShard(key);
  return AddSubrequest(impl_->Hdel(std::move(key), std::move(fields)));
}

RequestHexists MockTransaction::Hexists(std::string key, std::string field) {
  UpdateShard(key);
  return AddSubrequest(impl_->Hexists(std::move(key), std::move(field)));
}

RequestHget MockTransaction::Hget(std::string key, std::string field) {
  UpdateShard(key);
  return AddSubrequest(impl_->Hget(std::move(key), std::move(field)));
}

RequestHgetall MockTransaction::Hgetall(std::string key) {
  UpdateShard(key);
  return AddSubrequest(impl_->Hgetall(std::move(key)));
}

RequestHincrby MockTransaction::Hincrby(std::string key, std::string field,
                                        int64_t increment) {
  UpdateShard(key);
  return AddSubrequest(
      impl_->Hincrby(std::move(key), std::move(field), increment));
}

RequestHincrbyfloat MockTransaction::Hincrbyfloat(std::string key,
                                                  std::string field,
                                                  double increment) {
  UpdateShard(key);
  return AddSubrequest(
      impl_->Hincrbyfloat(std::move(key), std::move(field), increment));
}

RequestHkeys MockTransaction::Hkeys(std::string key) {
  UpdateShard(key);
  return AddSubrequest(impl_->Hkeys(std::move(key)));
}

RequestHlen MockTransaction::Hlen(std::string key) {
  UpdateShard(key);
  return AddSubrequest(impl_->Hlen(std::move(key)));
}

RequestHmget MockTransaction::Hmget(std::string key,
                                    std::vector<std::string> fields) {
  UpdateShard(key);
  return AddSubrequest(impl_->Hmget(std::move(key), std::move(fields)));
}

RequestHmset MockTransaction::Hmset(
    std::string key,
    std::vector<std::pair<std::string, std::string>> field_values) {
  UpdateShard(key);
  return AddSubrequest(impl_->Hmset(std::move(key), std::move(field_values)));
}

RequestHset MockTransaction::Hset(std::string key, std::string field,
                                  std::string value) {
  UpdateShard(key);
  return AddSubrequest(
      impl_->Hset(std::move(key), std::move(field), std::move(value)));
}

RequestHsetnx MockTransaction::Hsetnx(std::string key, std::string field,
                                      std::string value) {
  UpdateShard(key);
  return AddSubrequest(
      impl_->Hsetnx(std::move(key), std::move(field), std::move(value)));
}

RequestHvals MockTransaction::Hvals(std::string key) {
  UpdateShard(key);
  return AddSubrequest(impl_->Hvals(std::move(key)));
}

RequestIncr MockTransaction::Incr(std::string key) {
  UpdateShard(key);
  return AddSubrequest(impl_->Incr(std::move(key)));
}

RequestKeys MockTransaction::Keys(std::string keys_pattern, size_t shard) {
  UpdateShard(shard);
  return AddSubrequest(impl_->Keys(std::move(keys_pattern), shard));
}

RequestLindex MockTransaction::Lindex(std::string key, int64_t index) {
  UpdateShard(key);
  return AddSubrequest(impl_->Lindex(std::move(key), index));
}

RequestLlen MockTransaction::Llen(std::string key) {
  UpdateShard(key);
  return AddSubrequest(impl_->Llen(std::move(key)));
}

RequestLpop MockTransaction::Lpop(std::string key) {
  UpdateShard(key);
  return AddSubrequest(impl_->Lpop(std::move(key)));
}

RequestLpush MockTransaction::Lpush(std::string key, std::string value) {
  UpdateShard(key);
  return AddSubrequest(impl_->Lpush(std::move(key), std::move(value)));
}

RequestLpush MockTransaction::Lpush(std::string key,
                                    std::vector<std::string> values) {
  UpdateShard(key);
  return AddSubrequest(impl_->Lpush(std::move(key), std::move(values)));
}

RequestLpushx MockTransaction::Lpushx(std::string key, std::string element) {
  UpdateShard(key);
  return AddSubrequest(impl_->Lpushx(std::move(key), std::move(element)));
}

RequestLrange MockTransaction::Lrange(std::string key, int64_t start,
                                      int64_t stop) {
  UpdateShard(key);
  return AddSubrequest(impl_->Lrange(std::move(key), start, stop));
}

RequestLrem MockTransaction::Lrem(std::string key, int64_t count,
                                  std::string element) {
  UpdateShard(key);
  return AddSubrequest(impl_->Lrem(std::move(key), count, std::move(element)));
}

RequestLtrim MockTransaction::Ltrim(std::string key, int64_t start,
                                    int64_t stop) {
  UpdateShard(key);
  return AddSubrequest(impl_->Ltrim(std::move(key), start, stop));
}

RequestMget MockTransaction::Mget(std::vector<std::string> keys) {
  UpdateShard(keys);
  return AddSubrequest(impl_->Mget(std::move(keys)));
}

RequestMset MockTransaction::Mset(
    std::vector<std::pair<std::string, std::string>> key_values) {
  UpdateShard(key_values);
  return AddSubrequest(impl_->Mset(std::move(key_values)));
}

RequestPersist MockTransaction::Persist(std::string key) {
  UpdateShard(key);
  return AddSubrequest(impl_->Persist(std::move(key)));
}

RequestPexpire MockTransaction::Pexpire(std::string key,
                                        std::chrono::milliseconds ttl) {
  UpdateShard(key);
  return AddSubrequest(impl_->Pexpire(std::move(key), ttl));
}

RequestPing MockTransaction::Ping(size_t shard) {
  UpdateShard(shard);
  return AddSubrequest(impl_->Ping(shard));
}

RequestPingMessage MockTransaction::PingMessage(size_t shard,
                                                std::string message) {
  UpdateShard(shard);
  return AddSubrequest(impl_->PingMessage(shard, std::move(message)));
}

RequestRename MockTransaction::Rename(std::string key, std::string new_key) {
  UpdateShard(key);
  UpdateShard(new_key);
  return AddSubrequest(impl_->Rename(std::move(key), std::move(new_key)));
}

RequestRpop MockTransaction::Rpop(std::string key) {
  UpdateShard(key);
  return AddSubrequest(impl_->Rpop(std::move(key)));
}

RequestRpush MockTransaction::Rpush(std::string key, std::string value) {
  UpdateShard(key);
  return AddSubrequest(impl_->Rpush(std::move(key), std::move(value)));
}

RequestRpush MockTransaction::Rpush(std::string key,
                                    std::vector<std::string> values) {
  UpdateShard(key);
  return AddSubrequest(impl_->Rpush(std::move(key), std::move(values)));
}

RequestRpushx MockTransaction::Rpushx(std::string key, std::string element) {
  UpdateShard(key);
  return AddSubrequest(impl_->Rpushx(std::move(key), std::move(element)));
}

RequestSadd MockTransaction::Sadd(std::string key, std::string member) {
  UpdateShard(key);
  return AddSubrequest(impl_->Sadd(std::move(key), std::move(member)));
}

RequestSadd MockTransaction::Sadd(std::string key,
                                  std::vector<std::string> members) {
  UpdateShard(key);
  return AddSubrequest(impl_->Sadd(std::move(key), std::move(members)));
}

RequestScard MockTransaction::Scard(std::string key) {
  UpdateShard(key);
  return AddSubrequest(impl_->Scard(std::move(key)));
}

RequestSet MockTransaction::Set(std::string key, std::string value) {
  UpdateShard(key);
  return AddSubrequest(impl_->Set(std::move(key), std::move(value)));
}

RequestSet MockTransaction::Set(std::string key, std::string value,
                                std::chrono::milliseconds ttl) {
  UpdateShard(key);
  return AddSubrequest(impl_->Set(std::move(key), std::move(value), ttl));
}

RequestSetIfExist MockTransaction::SetIfExist(std::string key,
                                              std::string value) {
  UpdateShard(key);
  return AddSubrequest(impl_->SetIfExist(std::move(key), std::move(value)));
}

RequestSetIfExist MockTransaction::SetIfExist(std::string key,
                                              std::string value,
                                              std::chrono::milliseconds ttl) {
  UpdateShard(key);
  return AddSubrequest(
      impl_->SetIfExist(std::move(key), std::move(value), ttl));
}

RequestSetIfNotExist MockTransaction::SetIfNotExist(std::string key,
                                                    std::string value) {
  UpdateShard(key);
  return AddSubrequest(impl_->SetIfNotExist(std::move(key), std::move(value)));
}

RequestSetIfNotExist MockTransaction::SetIfNotExist(
    std::string key, std::string value, std::chrono::milliseconds ttl) {
  UpdateShard(key);
  return AddSubrequest(
      impl_->SetIfNotExist(std::move(key), std::move(value), ttl));
}

RequestSetex MockTransaction::Setex(std::string key,
                                    std::chrono::seconds seconds,
                                    std::string value) {
  UpdateShard(key);
  return AddSubrequest(impl_->Setex(std::move(key), seconds, std::move(value)));
}

RequestSismember MockTransaction::Sismember(std::string key,
                                            std::string member) {
  UpdateShard(key);
  return AddSubrequest(impl_->Sismember(std::move(key), std::move(member)));
}

RequestSmembers MockTransaction::Smembers(std::string key) {
  UpdateShard(key);
  return AddSubrequest(impl_->Smembers(std::move(key)));
}

RequestSrandmember MockTransaction::Srandmember(std::string key) {
  UpdateShard(key);
  return AddSubrequest(impl_->Srandmember(std::move(key)));
}

RequestSrandmembers MockTransaction::Srandmembers(std::string key,
                                                  int64_t count) {
  UpdateShard(key);
  return AddSubrequest(impl_->Srandmembers(std::move(key), count));
}

RequestSrem MockTransaction::Srem(std::string key, std::string member) {
  UpdateShard(key);
  return AddSubrequest(impl_->Srem(std::move(key), std::move(member)));
}

RequestSrem MockTransaction::Srem(std::string key,
                                  std::vector<std::string> members) {
  UpdateShard(key);
  return AddSubrequest(impl_->Srem(std::move(key), std::move(members)));
}

RequestStrlen MockTransaction::Strlen(std::string key) {
  UpdateShard(key);
  return AddSubrequest(impl_->Strlen(std::move(key)));
}

RequestTime MockTransaction::Time(size_t shard) {
  UpdateShard(shard);
  return AddSubrequest(impl_->Time(shard));
}

RequestTtl MockTransaction::Ttl(std::string key) {
  UpdateShard(key);
  return AddSubrequest(impl_->Ttl(std::move(key)));
}

RequestType MockTransaction::Type(std::string key) {
  UpdateShard(key);
  return AddSubrequest(impl_->Type(std::move(key)));
}

RequestZadd MockTransaction::Zadd(std::string key, double score,
                                  std::string member) {
  UpdateShard(key);
  return AddSubrequest(impl_->Zadd(std::move(key), score, std::move(member)));
}

RequestZadd MockTransaction::Zadd(std::string key, double score,
                                  std::string member,
                                  const ZaddOptions& options) {
  UpdateShard(key);
  return AddSubrequest(
      impl_->Zadd(std::move(key), score, std::move(member), options));
}

RequestZadd MockTransaction::Zadd(
    std::string key,
    std::vector<std::pair<double, std::string>> scored_members) {
  UpdateShard(key);
  return AddSubrequest(impl_->Zadd(std::move(key), std::move(scored_members)));
}

RequestZadd MockTransaction::Zadd(
    std::string key, std::vector<std::pair<double, std::string>> scored_members,
    const ZaddOptions& options) {
  UpdateShard(key);
  return AddSubrequest(
      impl_->Zadd(std::move(key), std::move(scored_members), options));
}

RequestZaddIncr MockTransaction::ZaddIncr(std::string key, double score,
                                          std::string member) {
  UpdateShard(key);
  return AddSubrequest(
      impl_->ZaddIncr(std::move(key), score, std::move(member)));
}

RequestZaddIncrExisting MockTransaction::ZaddIncrExisting(std::string key,
                                                          double score,
                                                          std::string member) {
  UpdateShard(key);
  return AddSubrequest(
      impl_->ZaddIncrExisting(std::move(key), score, std::move(member)));
}

RequestZcard MockTransaction::Zcard(std::string key) {
  UpdateShard(key);
  return AddSubrequest(impl_->Zcard(std::move(key)));
}

RequestZrange MockTransaction::Zrange(std::string key, int64_t start,
                                      int64_t stop) {
  UpdateShard(key);
  return AddSubrequest(impl_->Zrange(std::move(key), start, stop));
}

RequestZrangeWithScores MockTransaction::ZrangeWithScores(std::string key,
                                                          int64_t start,
                                                          int64_t stop) {
  UpdateShard(key);
  return AddSubrequest(impl_->ZrangeWithScores(std::move(key), start, stop));
}

RequestZrangebyscore MockTransaction::Zrangebyscore(std::string key, double min,
                                                    double max) {
  UpdateShard(key);
  return AddSubrequest(impl_->Zrangebyscore(std::move(key), min, max));
}

RequestZrangebyscore MockTransaction::Zrangebyscore(std::string key,
                                                    std::string min,
                                                    std::string max) {
  UpdateShard(key);
  return AddSubrequest(
      impl_->Zrangebyscore(std::move(key), std::move(min), std::move(max)));
}

RequestZrangebyscore MockTransaction::Zrangebyscore(
    std::string key, double min, double max,
    const RangeOptions& range_options) {
  UpdateShard(key);
  return AddSubrequest(
      impl_->Zrangebyscore(std::move(key), min, max, range_options));
}

RequestZrangebyscore MockTransaction::Zrangebyscore(
    std::string key, std::string min, std::string max,
    const RangeOptions& range_options) {
  UpdateShard(key);
  return AddSubrequest(impl_->Zrangebyscore(std::move(key), std::move(min),
                                            std::move(max), range_options));
}

RequestZrangebyscoreWithScores MockTransaction::ZrangebyscoreWithScores(
    std::string key, double min, double max) {
  UpdateShard(key);
  return AddSubrequest(
      impl_->ZrangebyscoreWithScores(std::move(key), min, max));
}

RequestZrangebyscoreWithScores MockTransaction::ZrangebyscoreWithScores(
    std::string key, std::string min, std::string max) {
  UpdateShard(key);
  return AddSubrequest(impl_->ZrangebyscoreWithScores(
      std::move(key), std::move(min), std::move(max)));
}

RequestZrangebyscoreWithScores MockTransaction::ZrangebyscoreWithScores(
    std::string key, double min, double max,
    const RangeOptions& range_options) {
  UpdateShard(key);
  return AddSubrequest(
      impl_->ZrangebyscoreWithScores(std::move(key), min, max, range_options));
}

RequestZrangebyscoreWithScores MockTransaction::ZrangebyscoreWithScores(
    std::string key, std::string min, std::string max,
    const RangeOptions& range_options) {
  UpdateShard(key);
  return AddSubrequest(impl_->ZrangebyscoreWithScores(
      std::move(key), std::move(min), std::move(max), range_options));
}

RequestZrem MockTransaction::Zrem(std::string key, std::string member) {
  UpdateShard(key);
  return AddSubrequest(impl_->Zrem(std::move(key), std::move(member)));
}

RequestZrem MockTransaction::Zrem(std::string key,
                                  std::vector<std::string> members) {
  UpdateShard(key);
  return AddSubrequest(impl_->Zrem(std::move(key), std::move(members)));
}

RequestZremrangebyrank MockTransaction::Zremrangebyrank(std::string key,
                                                        int64_t start,
                                                        int64_t stop) {
  UpdateShard(key);
  return AddSubrequest(impl_->Zremrangebyrank(std::move(key), start, stop));
}

RequestZremrangebyscore MockTransaction::Zremrangebyscore(std::string key,
                                                          double min,
                                                          double max) {
  UpdateShard(key);
  return AddSubrequest(impl_->Zremrangebyscore(std::move(key), min, max));
}

RequestZremrangebyscore MockTransaction::Zremrangebyscore(std::string key,
                                                          std::string min,
                                                          std::string max) {
  UpdateShard(key);
  return AddSubrequest(
      impl_->Zremrangebyscore(std::move(key), std::move(min), std::move(max)));
}

RequestZscore MockTransaction::Zscore(std::string key, std::string member) {
  UpdateShard(key);
  return AddSubrequest(impl_->Zscore(std::move(key), std::move(member)));
}

// end of redis commands

void MockTransaction::UpdateShard(const std::string& key) {
  try {
    UpdateShard(client_->ShardByKey(key));
  } catch (const USERVER_NAMESPACE::redis::InvalidArgumentException& ex) {
    throw USERVER_NAMESPACE::redis::InvalidArgumentException(
        ex.what() + std::string{" for key=" + key});
  }
}

void MockTransaction::UpdateShard(const std::vector<std::string>& keys) {
  for (const auto& key : keys) {
    UpdateShard(key);
  }
}

void MockTransaction::UpdateShard(
    const std::vector<std::pair<std::string, std::string>>& key_values) {
  for (const auto& key_value : key_values) {
    UpdateShard(key_value.first);
  }
}

void MockTransaction::UpdateShard(size_t shard) {
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
Request<Result, ReplyType> MockTransaction::AddSubrequest(
    Request<Result, ReplyType>&& subrequest) {
  engine::Promise<ReplyType> promise;
  Request<Result, ReplyType> request(
      std::make_unique<impl::TransactionSubrequestDataImpl<ReplyType>>(
          promise.get_future()));
  result_promises_.emplace_back(std::make_unique<ResultPromise>(
      std::move(promise), std::move(subrequest)));
  return request;
}

RequestExec MockTransaction::CreateMockExecRequest() {
  return RequestExec(
      std::make_unique<MockRequestExecDataImpl>(std::move(result_promises_)));
}

}  // namespace storages::redis

USERVER_NAMESPACE_END
