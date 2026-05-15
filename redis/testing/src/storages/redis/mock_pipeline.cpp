#include <userver/storages/redis/mock_pipeline.hpp>

#include <vector>

#include <userver/utils/assert.hpp>

#include <userver/storages/redis/impl/pipeline_subrequest_data.hpp>
#include <userver/storages/redis/mock_client_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

class MockPipeline::ResultPromise {
public:
    template <typename Result, typename ReplyType>
    ResultPromise(engine::Promise<ReplyType>&& promise, Request<Result, ReplyType>&& subrequest)
        : impl_(std::make_unique<ResultPromiseImpl<Result, ReplyType>>(std::move(promise), std::move(subrequest))) {}

    ResultPromise(ResultPromise&& other) = default;

    void ProcessReply(const std::string& request_description) { impl_->ProcessReply(request_description); }

private:
    class ResultPromiseImplBase {
    public:
        virtual ~ResultPromiseImplBase() = default;

        virtual void ProcessReply(const std::string& request_description) = 0;
    };

    template <typename Result, typename ReplyType>
    class ResultPromiseImpl : public ResultPromiseImplBase {
    public:
        ResultPromiseImpl(engine::Promise<ReplyType>&& promise, Request<Result, ReplyType>&& subrequest)
            : promise_(std::move(promise)), subrequest_(std::move(subrequest)) {}

        void ProcessReply(const std::string& request_description) override {
            try {
                if constexpr (std::same_as<ReplyType, void>) {
                    subrequest_.Get(request_description);
                    promise_.set_value();
                } else {
                    promise_.set_value(subrequest_.Get(request_description));
                }
            } catch (const RequestFailedException&) {
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

class MockPipeline::MockRequestExecDataImpl final : public RequestDataBase<void> {
public:
    MockRequestExecDataImpl(std::vector<std::unique_ptr<ResultPromise>>&& result_promises)
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

    engine::impl::ContextAccessor* TryGetContextAccessor() noexcept override {
        UASSERT_MSG(false, "not supported in mocked request");
        return nullptr;
    }

private:
    std::vector<std::unique_ptr<ResultPromise>> result_promises_;
};

MockPipeline::MockPipeline(
    std::shared_ptr<MockClientBase> client,
    std::unique_ptr<MockPipelineImplBase> impl,
    CheckShards check_shards
)
    : client_(std::move(client)), check_shards_(check_shards), impl_(std::move(impl)) {}

MockPipeline::~MockPipeline() = default;

RequestExec MockPipeline::Exec(const CommandControl& command_control) {
    if (!shard_) {
        throw EmptyPipelineException("Can't determine shard. Empty pipeline?");
    }
    if (command_control.force_shard_idx) {
        shard_ = command_control.force_shard_idx;
    }
    client_->CheckShardIdx(*shard_);
    return CreateMockExecRequest();
}

// redis commands:

RequestAppend MockPipeline::Append(std::string key, std::string value) {
    UpdateShard(key);
    return AddSubrequest(impl_->Append(std::move(key), std::move(value)));
}

RequestBitop MockPipeline::Bitop(BitOperation op, std::string dest, std::vector<std::string> srcs) {
    UpdateShard(dest);
    return AddSubrequest(impl_->Bitop(op, std::move(dest), std::move(srcs)));
}

RequestDbsize MockPipeline::Dbsize(size_t shard) {
    UpdateShard(shard);
    return AddSubrequest(impl_->Dbsize(shard));
}

RequestDecr MockPipeline::Decr(std::string key) {
    UpdateShard(key);
    return AddSubrequest(impl_->Decr(std::move(key)));
}

RequestDel MockPipeline::Del(std::string key) {
    UpdateShard(key);
    return AddSubrequest(impl_->Del(std::move(key)));
}

RequestDel MockPipeline::Del(std::vector<std::string> keys) {
    UpdateShard(keys);
    return AddSubrequest(impl_->Del(std::move(keys)));
}

RequestUnlink MockPipeline::Unlink(std::string key) {
    UpdateShard(key);
    return AddSubrequest(impl_->Unlink(std::move(key)));
}

RequestUnlink MockPipeline::Unlink(std::vector<std::string> keys) {
    UpdateShard(keys);
    return AddSubrequest(impl_->Unlink(std::move(keys)));
}

RequestExists MockPipeline::Exists(std::string key) {
    UpdateShard(key);
    return AddSubrequest(impl_->Exists(std::move(key)));
}

RequestExists MockPipeline::Exists(std::vector<std::string> keys) {
    UpdateShard(keys);
    return AddSubrequest(impl_->Exists(std::move(keys)));
}

RequestExpire MockPipeline::Expire(std::string key, std::chrono::seconds ttl) {
    UpdateShard(key);
    return AddSubrequest(impl_->Expire(std::move(key), ttl));
}

RequestExpire MockPipeline::Expire(std::string key, std::chrono::seconds ttl, ExpireOptions options) {
    UpdateShard(key);
    return AddSubrequest(impl_->Expire(std::move(key), ttl, options));
}

RequestGeoadd MockPipeline::Geoadd(std::string key, GeoaddArg point_member) {
    UpdateShard(key);
    return AddSubrequest(impl_->Geoadd(std::move(key), point_member));
}

RequestGeoadd MockPipeline::Geoadd(std::string key, std::vector<GeoaddArg> point_members) {
    UpdateShard(key);
    return AddSubrequest(impl_->Geoadd(std::move(key), std::move(point_members)));
}

RequestGeopos MockPipeline::Geopos(std::string key, std::vector<std::string> members) {
    UpdateShard(key);
    return AddSubrequest(impl_->Geopos(std::move(key), std::move(members)));
}

RequestGeoradius MockPipeline::Georadius(
    std::string key,
    Longitude lon,
    Latitude lat,
    double radius,
    const GeoradiusOptions& georadius_options
) {
    UpdateShard(key);
    return AddSubrequest(impl_->Georadius(std::move(key), lon, lat, radius, georadius_options));
}

RequestGeosearch MockPipeline::Geosearch(
    std::string key,
    std::string member,
    double radius,
    const GeosearchOptions& geosearch_options
) {
    UpdateShard(key);
    return AddSubrequest(impl_->Geosearch(std::move(key), std::move(member), radius, geosearch_options));
}

RequestGeosearch MockPipeline::Geosearch(
    std::string key,
    std::string member,
    BoxWidth width,
    BoxHeight height,
    const GeosearchOptions& geosearch_options
) {
    UpdateShard(key);
    return AddSubrequest(impl_->Geosearch(std::move(key), std::move(member), width, height, geosearch_options));
}

RequestGeosearch MockPipeline::Geosearch(
    std::string key,
    Longitude lon,
    Latitude lat,
    double radius,
    const GeosearchOptions& geosearch_options
) {
    UpdateShard(key);
    return AddSubrequest(impl_->Geosearch(std::move(key), lon, lat, radius, geosearch_options));
}

RequestGeosearch MockPipeline::Geosearch(
    std::string key,
    Longitude lon,
    Latitude lat,
    BoxWidth width,
    BoxHeight height,
    const GeosearchOptions& geosearch_options
) {
    UpdateShard(key);
    return AddSubrequest(impl_->Geosearch(std::move(key), lon, lat, width, height, geosearch_options));
}

RequestGet MockPipeline::Get(std::string key) {
    UpdateShard(key);
    return AddSubrequest(impl_->Get(std::move(key)));
}

RequestGetset MockPipeline::Getset(std::string key, std::string value) {
    UpdateShard(key);
    return AddSubrequest(impl_->Getset(std::move(key), std::move(value)));
}

RequestHdel MockPipeline::Hdel(std::string key, std::string field) {
    UpdateShard(key);
    return AddSubrequest(impl_->Hdel(std::move(key), std::move(field)));
}

RequestHdel MockPipeline::Hdel(std::string key, std::vector<std::string> fields) {
    UpdateShard(key);
    return AddSubrequest(impl_->Hdel(std::move(key), std::move(fields)));
}

RequestHexists MockPipeline::Hexists(std::string key, std::string field) {
    UpdateShard(key);
    return AddSubrequest(impl_->Hexists(std::move(key), std::move(field)));
}

RequestHget MockPipeline::Hget(std::string key, std::string field) {
    UpdateShard(key);
    return AddSubrequest(impl_->Hget(std::move(key), std::move(field)));
}

RequestHgetall MockPipeline::Hgetall(std::string key) {
    UpdateShard(key);
    return AddSubrequest(impl_->Hgetall(std::move(key)));
}

RequestHincrby MockPipeline::Hincrby(std::string key, std::string field, int64_t increment) {
    UpdateShard(key);
    return AddSubrequest(impl_->Hincrby(std::move(key), std::move(field), increment));
}

RequestHincrbyfloat MockPipeline::Hincrbyfloat(std::string key, std::string field, double increment) {
    UpdateShard(key);
    return AddSubrequest(impl_->Hincrbyfloat(std::move(key), std::move(field), increment));
}

RequestHkeys MockPipeline::Hkeys(std::string key) {
    UpdateShard(key);
    return AddSubrequest(impl_->Hkeys(std::move(key)));
}

RequestHlen MockPipeline::Hlen(std::string key) {
    UpdateShard(key);
    return AddSubrequest(impl_->Hlen(std::move(key)));
}

RequestHmget MockPipeline::Hmget(std::string key, std::vector<std::string> fields) {
    UpdateShard(key);
    return AddSubrequest(impl_->Hmget(std::move(key), std::move(fields)));
}

RequestHmset MockPipeline::Hmset(std::string key, std::vector<std::pair<std::string, std::string>> field_values) {
    UpdateShard(key);
    return AddSubrequest(impl_->Hmset(std::move(key), std::move(field_values)));
}

RequestHset MockPipeline::Hset(std::string key, std::string field, std::string value) {
    UpdateShard(key);
    return AddSubrequest(impl_->Hset(std::move(key), std::move(field), std::move(value)));
}

RequestHsetnx MockPipeline::Hsetnx(std::string key, std::string field, std::string value) {
    UpdateShard(key);
    return AddSubrequest(impl_->Hsetnx(std::move(key), std::move(field), std::move(value)));
}

RequestHvals MockPipeline::Hvals(std::string key) {
    UpdateShard(key);
    return AddSubrequest(impl_->Hvals(std::move(key)));
}

RequestIncr MockPipeline::Incr(std::string key) {
    UpdateShard(key);
    return AddSubrequest(impl_->Incr(std::move(key)));
}

RequestKeys MockPipeline::Keys(std::string keys_pattern, size_t shard) {
    UpdateShard(shard);
    return AddSubrequest(impl_->Keys(std::move(keys_pattern), shard));
}

RequestLindex MockPipeline::Lindex(std::string key, int64_t index) {
    UpdateShard(key);
    return AddSubrequest(impl_->Lindex(std::move(key), index));
}

RequestLlen MockPipeline::Llen(std::string key) {
    UpdateShard(key);
    return AddSubrequest(impl_->Llen(std::move(key)));
}

RequestLpop MockPipeline::Lpop(std::string key) {
    UpdateShard(key);
    return AddSubrequest(impl_->Lpop(std::move(key)));
}

RequestLpush MockPipeline::Lpush(std::string key, std::string value) {
    UpdateShard(key);
    return AddSubrequest(impl_->Lpush(std::move(key), std::move(value)));
}

RequestLpush MockPipeline::Lpush(std::string key, std::vector<std::string> values) {
    UpdateShard(key);
    return AddSubrequest(impl_->Lpush(std::move(key), std::move(values)));
}

RequestLpushx MockPipeline::Lpushx(std::string key, std::string element) {
    UpdateShard(key);
    return AddSubrequest(impl_->Lpushx(std::move(key), std::move(element)));
}

RequestLrange MockPipeline::Lrange(std::string key, int64_t start, int64_t stop) {
    UpdateShard(key);
    return AddSubrequest(impl_->Lrange(std::move(key), start, stop));
}

RequestLrem MockPipeline::Lrem(std::string key, int64_t count, std::string element) {
    UpdateShard(key);
    return AddSubrequest(impl_->Lrem(std::move(key), count, std::move(element)));
}

RequestLtrim MockPipeline::Ltrim(std::string key, int64_t start, int64_t stop) {
    UpdateShard(key);
    return AddSubrequest(impl_->Ltrim(std::move(key), start, stop));
}

RequestMget MockPipeline::Mget(std::vector<std::string> keys) {
    UpdateShard(keys);
    return AddSubrequest(impl_->Mget(std::move(keys)));
}

RequestMset MockPipeline::Mset(std::vector<std::pair<std::string, std::string>> key_values) {
    UpdateShard(key_values);
    return AddSubrequest(impl_->Mset(std::move(key_values)));
}

RequestPersist MockPipeline::Persist(std::string key) {
    UpdateShard(key);
    return AddSubrequest(impl_->Persist(std::move(key)));
}

RequestPexpire MockPipeline::Pexpire(std::string key, std::chrono::milliseconds ttl) {
    UpdateShard(key);
    return AddSubrequest(impl_->Pexpire(std::move(key), ttl));
}

RequestPing MockPipeline::Ping(size_t shard) {
    UpdateShard(shard);
    return AddSubrequest(impl_->Ping(shard));
}

RequestPingMessage MockPipeline::PingMessage(size_t shard, std::string message) {
    UpdateShard(shard);
    return AddSubrequest(impl_->PingMessage(shard, std::move(message)));
}

RequestRename MockPipeline::Rename(std::string key, std::string new_key) {
    UpdateShard(key);
    UpdateShard(new_key);
    return AddSubrequest(impl_->Rename(std::move(key), std::move(new_key)));
}

RequestRpop MockPipeline::Rpop(std::string key) {
    UpdateShard(key);
    return AddSubrequest(impl_->Rpop(std::move(key)));
}

RequestRpush MockPipeline::Rpush(std::string key, std::string value) {
    UpdateShard(key);
    return AddSubrequest(impl_->Rpush(std::move(key), std::move(value)));
}

RequestRpush MockPipeline::Rpush(std::string key, std::vector<std::string> values) {
    UpdateShard(key);
    return AddSubrequest(impl_->Rpush(std::move(key), std::move(values)));
}

RequestRpushx MockPipeline::Rpushx(std::string key, std::string element) {
    UpdateShard(key);
    return AddSubrequest(impl_->Rpushx(std::move(key), std::move(element)));
}

RequestSadd MockPipeline::Sadd(std::string key, std::string member) {
    UpdateShard(key);
    return AddSubrequest(impl_->Sadd(std::move(key), std::move(member)));
}

RequestSadd MockPipeline::Sadd(std::string key, std::vector<std::string> members) {
    UpdateShard(key);
    return AddSubrequest(impl_->Sadd(std::move(key), std::move(members)));
}

RequestScard MockPipeline::Scard(std::string key) {
    UpdateShard(key);
    return AddSubrequest(impl_->Scard(std::move(key)));
}

RequestSet MockPipeline::Set(std::string key, std::string value) {
    UpdateShard(key);
    return AddSubrequest(impl_->Set(std::move(key), std::move(value)));
}

RequestSet MockPipeline::Set(std::string key, std::string value, std::chrono::milliseconds ttl) {
    UpdateShard(key);
    return AddSubrequest(impl_->Set(std::move(key), std::move(value), ttl));
}

RequestSetIfExist MockPipeline::SetIfExist(std::string key, std::string value) {
    UpdateShard(key);
    return AddSubrequest(impl_->SetIfExist(std::move(key), std::move(value)));
}

RequestSetIfExist MockPipeline::SetIfExist(std::string key, std::string value, std::chrono::milliseconds ttl) {
    UpdateShard(key);
    return AddSubrequest(impl_->SetIfExist(std::move(key), std::move(value), ttl));
}

RequestSetIfNotExist MockPipeline::SetIfNotExist(std::string key, std::string value) {
    UpdateShard(key);
    return AddSubrequest(impl_->SetIfNotExist(std::move(key), std::move(value)));
}

RequestSetIfNotExist MockPipeline::SetIfNotExist(std::string key, std::string value, std::chrono::milliseconds ttl) {
    UpdateShard(key);
    return AddSubrequest(impl_->SetIfNotExist(std::move(key), std::move(value), ttl));
}

RequestSetIfNotExistOrGet MockPipeline::SetIfNotExistOrGet(std::string key, std::string value) {
    UpdateShard(key);
    return AddSubrequest(impl_->SetIfNotExistOrGet(std::move(key), std::move(value)));
}

RequestSetIfNotExistOrGet MockPipeline::SetIfNotExistOrGet(
    std::string key,
    std::string value,
    std::chrono::milliseconds ttl
) {
    UpdateShard(key);
    return AddSubrequest(impl_->SetIfNotExistOrGet(std::move(key), std::move(value), ttl));
}

RequestSetex MockPipeline::Setex(std::string key, std::chrono::seconds seconds, std::string value) {
    UpdateShard(key);
    return AddSubrequest(impl_->Setex(std::move(key), seconds, std::move(value)));
}

RequestSismember MockPipeline::Sismember(std::string key, std::string member) {
    UpdateShard(key);
    return AddSubrequest(impl_->Sismember(std::move(key), std::move(member)));
}

RequestSmembers MockPipeline::Smembers(std::string key) {
    UpdateShard(key);
    return AddSubrequest(impl_->Smembers(std::move(key)));
}

RequestSrandmember MockPipeline::Srandmember(std::string key) {
    UpdateShard(key);
    return AddSubrequest(impl_->Srandmember(std::move(key)));
}

RequestSrandmembers MockPipeline::Srandmembers(std::string key, int64_t count) {
    UpdateShard(key);
    return AddSubrequest(impl_->Srandmembers(std::move(key), count));
}

RequestSrem MockPipeline::Srem(std::string key, std::string member) {
    UpdateShard(key);
    return AddSubrequest(impl_->Srem(std::move(key), std::move(member)));
}

RequestSrem MockPipeline::Srem(std::string key, std::vector<std::string> members) {
    UpdateShard(key);
    return AddSubrequest(impl_->Srem(std::move(key), std::move(members)));
}

RequestStrlen MockPipeline::Strlen(std::string key) {
    UpdateShard(key);
    return AddSubrequest(impl_->Strlen(std::move(key)));
}

RequestTime MockPipeline::Time(size_t shard) {
    UpdateShard(shard);
    return AddSubrequest(impl_->Time(shard));
}

RequestTtl MockPipeline::Ttl(std::string key) {
    UpdateShard(key);
    return AddSubrequest(impl_->Ttl(std::move(key)));
}

RequestType MockPipeline::Type(std::string key) {
    UpdateShard(key);
    return AddSubrequest(impl_->Type(std::move(key)));
}

RequestZadd MockPipeline::Zadd(std::string key, double score, std::string member) {
    UpdateShard(key);
    return AddSubrequest(impl_->Zadd(std::move(key), score, std::move(member)));
}

RequestZadd MockPipeline::Zadd(std::string key, double score, std::string member, const ZaddOptions& options) {
    UpdateShard(key);
    return AddSubrequest(impl_->Zadd(std::move(key), score, std::move(member), options));
}

RequestZadd MockPipeline::Zadd(std::string key, std::vector<std::pair<double, std::string>> scored_members) {
    UpdateShard(key);
    return AddSubrequest(impl_->Zadd(std::move(key), std::move(scored_members)));
}

RequestZadd MockPipeline::Zadd(
    std::string key,
    std::vector<std::pair<double, std::string>> scored_members,
    const ZaddOptions& options
) {
    UpdateShard(key);
    return AddSubrequest(impl_->Zadd(std::move(key), std::move(scored_members), options));
}

RequestZaddIncr MockPipeline::ZaddIncr(std::string key, double score, std::string member) {
    UpdateShard(key);
    return AddSubrequest(impl_->ZaddIncr(std::move(key), score, std::move(member)));
}

RequestZaddIncrExisting MockPipeline::ZaddIncrExisting(std::string key, double score, std::string member) {
    UpdateShard(key);
    return AddSubrequest(impl_->ZaddIncrExisting(std::move(key), score, std::move(member)));
}

RequestZcard MockPipeline::Zcard(std::string key) {
    UpdateShard(key);
    return AddSubrequest(impl_->Zcard(std::move(key)));
}

RequestZcount MockPipeline::Zcount(std::string key, double min, double max) {
    UpdateShard(key);
    return AddSubrequest(impl_->Zcount(std::move(key), min, max));
}

RequestZrange MockPipeline::Zrange(std::string key, int64_t start, int64_t stop) {
    UpdateShard(key);
    return AddSubrequest(impl_->Zrange(std::move(key), start, stop));
}

RequestZrangeWithScores MockPipeline::ZrangeWithScores(std::string key, int64_t start, int64_t stop) {
    UpdateShard(key);
    return AddSubrequest(impl_->ZrangeWithScores(std::move(key), start, stop));
}

RequestZrangebyscore MockPipeline::Zrangebyscore(std::string key, double min, double max) {
    UpdateShard(key);
    return AddSubrequest(impl_->Zrangebyscore(std::move(key), min, max));
}

RequestZrangebyscore MockPipeline::Zrangebyscore(std::string key, std::string min, std::string max) {
    UpdateShard(key);
    return AddSubrequest(impl_->Zrangebyscore(std::move(key), std::move(min), std::move(max)));
}

RequestZrangebyscore MockPipeline::Zrangebyscore(
    std::string key,
    double min,
    double max,
    const RangeOptions& range_options
) {
    UpdateShard(key);
    return AddSubrequest(impl_->Zrangebyscore(std::move(key), min, max, range_options));
}

RequestZrangebyscore MockPipeline::Zrangebyscore(
    std::string key,
    std::string min,
    std::string max,
    const RangeOptions& range_options
) {
    UpdateShard(key);
    return AddSubrequest(impl_->Zrangebyscore(std::move(key), std::move(min), std::move(max), range_options));
}

RequestZrangebyscoreWithScores MockPipeline::ZrangebyscoreWithScores(std::string key, double min, double max) {
    UpdateShard(key);
    return AddSubrequest(impl_->ZrangebyscoreWithScores(std::move(key), min, max));
}

RequestZrangebyscoreWithScores MockPipeline::ZrangebyscoreWithScores(
    std::string key,
    std::string min,
    std::string max
) {
    UpdateShard(key);
    return AddSubrequest(impl_->ZrangebyscoreWithScores(std::move(key), std::move(min), std::move(max)));
}

RequestZrangebyscoreWithScores MockPipeline::ZrangebyscoreWithScores(
    std::string key,
    double min,
    double max,
    const RangeOptions& range_options
) {
    UpdateShard(key);
    return AddSubrequest(impl_->ZrangebyscoreWithScores(std::move(key), min, max, range_options));
}

RequestZrangebyscoreWithScores MockPipeline::ZrangebyscoreWithScores(
    std::string key,
    std::string min,
    std::string max,
    const RangeOptions& range_options
) {
    UpdateShard(key);
    return AddSubrequest(impl_->ZrangebyscoreWithScores(std::move(key), std::move(min), std::move(max), range_options));
}

RequestZrem MockPipeline::Zrem(std::string key, std::string member) {
    UpdateShard(key);
    return AddSubrequest(impl_->Zrem(std::move(key), std::move(member)));
}

RequestZrem MockPipeline::Zrem(std::string key, std::vector<std::string> members) {
    UpdateShard(key);
    return AddSubrequest(impl_->Zrem(std::move(key), std::move(members)));
}

RequestZremrangebyrank MockPipeline::Zremrangebyrank(std::string key, int64_t start, int64_t stop) {
    UpdateShard(key);
    return AddSubrequest(impl_->Zremrangebyrank(std::move(key), start, stop));
}

RequestZremrangebyscore MockPipeline::Zremrangebyscore(std::string key, double min, double max) {
    UpdateShard(key);
    return AddSubrequest(impl_->Zremrangebyscore(std::move(key), min, max));
}

RequestZremrangebyscore MockPipeline::Zremrangebyscore(std::string key, std::string min, std::string max) {
    UpdateShard(key);
    return AddSubrequest(impl_->Zremrangebyscore(std::move(key), std::move(min), std::move(max)));
}

RequestZscore MockPipeline::Zscore(std::string key, std::string member) {
    UpdateShard(key);
    return AddSubrequest(impl_->Zscore(std::move(key), std::move(member)));
}

// end of redis commands

void MockPipeline::UpdateShard(const std::string& key) {
    try {
        UpdateShard(client_->ShardByKey(key));
    } catch (const InvalidArgumentException& ex) {
        throw InvalidArgumentException(ex.what() + std::string{" for key=" + key});
    }
}

void MockPipeline::UpdateShard(const std::vector<std::string>& keys) {
    for (const auto& key : keys) {
        UpdateShard(key);
    }
}

void MockPipeline::UpdateShard(const std::vector<std::pair<std::string, std::string>>& key_values) {
    for (const auto& key_value : key_values) {
        UpdateShard(key_value.first);
    }
}

void MockPipeline::UpdateShard(size_t shard) {
    if (shard_) {
        if (check_shards_ == CheckShards::kSame && *shard_ != shard) {
            std::ostringstream os;
            os << "storages::redis::Transaction must deal with the same shard across "
                  "all the operations. Shard="
               << *shard_
               << " was detected by first command, but one of the commands used "
                  "shard="
               << shard;
            throw InvalidArgumentException(os.str());
        }
    } else {
        shard_ = shard;
    }
}

template <typename Result, typename ReplyType>
Request<Result, ReplyType> MockPipeline::AddSubrequest(Request<Result, ReplyType>&& subrequest) {
    engine::Promise<ReplyType> promise;
    Request<Result, ReplyType>
        request(std::make_unique<impl::PipelineSubrequestDataImpl<ReplyType>>(promise.get_future()));
    result_promises_.emplace_back(std::make_unique<ResultPromise>(std::move(promise), std::move(subrequest)));
    return request;
}

RequestExec MockPipeline::CreateMockExecRequest() {
    return RequestExec(std::make_unique<MockRequestExecDataImpl>(std::move(result_promises_)));
}

}  // namespace storages::redis

USERVER_NAMESPACE_END
