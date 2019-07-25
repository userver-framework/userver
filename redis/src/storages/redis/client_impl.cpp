#include <storages/redis/client_impl.hpp>

#include <redis/sentinel.hpp>
#include <utils/assert.hpp>

#include "request_impl.hpp"

namespace storages {
namespace redis {

ClientImpl::ClientImpl(std::shared_ptr<::redis::Sentinel> sentinel)
    : redis_client_(std::move(sentinel)) {}

size_t ClientImpl::ShardsCount() const { return redis_client_->ShardsCount(); }

size_t ClientImpl::ShardByKey(const std::string& key) const {
  return redis_client_->ShardByKey(key);
}

const std::string& ClientImpl::GetAnyKeyForShard(size_t shard_idx) const {
  return redis_client_->GetAnyKeyForShard(shard_idx);
}

// redis commands:

RequestAppend ClientImpl::Append(std::string key, std::string value,
                                 const CommandControl& command_control) {
  auto shard = ShardByKey(key);
  return CreateRequest<RequestAppend>(
      MakeRequest(CmdArgs{"append", std::move(key), std::move(value)}, shard,
                  true, GetCommandControl(command_control)));
}

RequestDbsize ClientImpl::Dbsize(size_t shard,
                                 const CommandControl& command_control) {
  return CreateRequest<RequestDbsize>(MakeRequest(
      CmdArgs{"dbsize"}, shard, false, GetCommandControl(command_control)));
}

RequestDel ClientImpl::Del(std::string key,
                           const CommandControl& command_control) {
  auto shard = ShardByKey(key);
  return CreateRequest<RequestDel>(
      MakeRequest(CmdArgs{"del", std::move(key)}, shard, true,
                  GetCommandControl(command_control)));
}

RequestDel ClientImpl::Del(std::vector<std::string> keys,
                           const CommandControl& command_control) {
  if (keys.empty())
    return CreateDummyRequest<RequestDel>(
        std::make_shared<::redis::Reply>("del", 0));
  auto shard = ShardByKey(keys.at(0));
  return CreateRequest<RequestDel>(
      MakeRequest(CmdArgs{"del", std::move(keys)}, shard, true,
                  GetCommandControl(command_control)));
}

RequestEval ClientImpl::Eval(std::string script, std::vector<std::string> keys,
                             std::vector<std::string> args,
                             const CommandControl& command_control) {
  UASSERT(!keys.empty());
  auto shard = ShardByKey(keys.at(0));
  size_t keys_size = keys.size();
  return CreateRequest<RequestEval>(
      MakeRequest(CmdArgs{"eval", std::move(script), keys_size, std::move(keys),
                          std::move(args)},
                  shard, true, GetCommandControl(command_control)));
}

RequestExists ClientImpl::Exists(std::string key,
                                 const CommandControl& command_control) {
  auto shard = ShardByKey(key);
  return CreateRequest<RequestExists>(
      MakeRequest(CmdArgs{"exists", std::move(key)}, shard, false,
                  GetCommandControl(command_control)));
}

RequestExists ClientImpl::Exists(std::vector<std::string> keys,
                                 const CommandControl& command_control) {
  if (keys.empty())
    return CreateDummyRequest<RequestExists>(
        std::make_shared<::redis::Reply>("exists", 0));
  auto shard = ShardByKey(keys.at(0));
  return CreateRequest<RequestExists>(
      MakeRequest(CmdArgs{"exists", std::move(keys)}, shard, false,
                  GetCommandControl(command_control)));
}

RequestGet ClientImpl::Get(std::string key,
                           const CommandControl& command_control) {
  auto shard = ShardByKey(key);
  return CreateRequest<RequestGet>(
      MakeRequest(CmdArgs{"get", std::move(key)}, shard, false,
                  GetCommandControl(command_control)));
}

RequestHdel ClientImpl::Hdel(std::string key, std::string field,
                             const CommandControl& command_control) {
  auto shard = ShardByKey(key);
  return CreateRequest<RequestHdel>(
      MakeRequest(CmdArgs{"hdel", std::move(key), std::move(field)}, shard,
                  true, GetCommandControl(command_control)));
}

RequestHdel ClientImpl::Hdel(std::string key, std::vector<std::string> fields,
                             const CommandControl& command_control) {
  if (fields.empty())
    return CreateDummyRequest<RequestHdel>(
        std::make_shared<::redis::Reply>("hdel", 0));
  auto shard = ShardByKey(key);
  return CreateRequest<RequestHdel>(
      MakeRequest(CmdArgs{"hdel", std::move(key), std::move(fields)}, shard,
                  true, GetCommandControl(command_control)));
}

RequestHget ClientImpl::Hget(std::string key, std::string field,
                             const CommandControl& command_control) {
  auto shard = ShardByKey(key);
  return CreateRequest<RequestHget>(
      MakeRequest(CmdArgs{"hget", std::move(key), std::move(field)}, shard,
                  false, GetCommandControl(command_control)));
}

RequestHgetall ClientImpl::Hgetall(std::string key,
                                   const CommandControl& command_control) {
  auto shard = ShardByKey(key);
  return CreateRequest<RequestHgetall>(
      MakeRequest(CmdArgs{"hgetall", std::move(key)}, shard, false,
                  GetCommandControl(command_control)));
}

RequestHmset ClientImpl::Hmset(
    std::string key,
    std::vector<std::pair<std::string, std::string>> field_values,
    const CommandControl& command_control) {
  if (field_values.empty())
    return CreateDummyRequest<RequestHmset>(std::make_shared<::redis::Reply>(
        "hmset", ::redis::ReplyData::CreateStatus("OK")));
  auto shard = ShardByKey(key);
  return CreateRequest<RequestHmset>(
      MakeRequest(CmdArgs{"hmset", std::move(key), std::move(field_values)},
                  shard, true, GetCommandControl(command_control)));
}

RequestHset ClientImpl::Hset(std::string key, std::string field,
                             std::string value,
                             const CommandControl& command_control) {
  auto shard = ShardByKey(key);
  return CreateRequest<RequestHset>(MakeRequest(
      CmdArgs{"hset", std::move(key), std::move(field), std::move(value)},
      shard, true, GetCommandControl(command_control)));
}

RequestKeys ClientImpl::Keys(std::string keys_pattern, size_t shard,
                             const CommandControl& command_control) {
  return CreateRequest<RequestKeys>(
      MakeRequest(CmdArgs{"keys", std::move(keys_pattern)}, shard, false,
                  GetCommandControl(command_control)));
}

RequestMget ClientImpl::Mget(std::vector<std::string> keys,
                             const CommandControl& command_control) {
  if (keys.empty())
    return CreateDummyRequest<RequestMget>(
        std::make_shared<::redis::Reply>("mget", ::redis::ReplyData::Array{}));
  auto shard = ShardByKey(keys.at(0));
  return CreateRequest<RequestMget>(
      MakeRequest(CmdArgs{"mget", std::move(keys)}, shard, false,
                  GetCommandControl(command_control)));
}

RequestPing ClientImpl::Ping(size_t shard,
                             const CommandControl& command_control) {
  return CreateRequest<RequestPing>(MakeRequest(
      CmdArgs{"ping"}, shard, false, GetCommandControl(command_control)));
}

RequestPingMessage ClientImpl::Ping(size_t shard, std::string message,
                                    const CommandControl& command_control) {
  return CreateRequest<RequestPingMessage>(
      MakeRequest(CmdArgs{"ping", std::move(message)}, shard, false,
                  GetCommandControl(command_control)));
}

void ClientImpl::Publish(std::string channel, std::string message,
                         const CommandControl& command_control) {
  return Publish(std::move(channel), std::move(message), command_control,
                 PubShard::kZeroShard);
}

void ClientImpl::Publish(std::string channel, std::string message,
                         const CommandControl& command_control,
                         PubShard policy) {
  auto shard = GetPublishShard(policy);
  MakeRequest(CmdArgs{"publish", std::move(channel), std::move(message)}, shard,
              true, GetCommandControl(command_control));
}

RequestSet ClientImpl::Set(std::string key, std::string value,
                           const CommandControl& command_control) {
  auto shard = ShardByKey(key);
  return CreateRequest<RequestSet>(
      MakeRequest(CmdArgs{"set", std::move(key), std::move(value)}, shard, true,
                  GetCommandControl(command_control)));
}

RequestSet ClientImpl::Set(std::string key, std::string value,
                           std::chrono::milliseconds ttl,
                           const CommandControl& command_control) {
  auto shard = ShardByKey(key);
  return CreateRequest<RequestSet>(MakeRequest(
      CmdArgs{"set", std::move(key), std::move(value), "PX", ttl.count()},
      shard, true, GetCommandControl(command_control)));
}

RequestSetIfExist ClientImpl::SetIfExist(
    std::string key, std::string value, const CommandControl& command_control) {
  auto shard = ShardByKey(key);
  return CreateRequest<RequestSetIfExist>(
      MakeRequest(CmdArgs{"set", std::move(key), std::move(value), "XX"}, shard,
                  true, GetCommandControl(command_control)));
}

RequestSetIfExist ClientImpl::SetIfExist(
    std::string key, std::string value, std::chrono::milliseconds ttl,
    const CommandControl& command_control) {
  auto shard = ShardByKey(key);
  return CreateRequest<RequestSetIfExist>(MakeRequest(
      CmdArgs{"set", std::move(key), std::move(value), "PX", ttl.count(), "XX"},
      shard, true, GetCommandControl(command_control)));
}

RequestSetIfNotExist ClientImpl::SetIfNotExist(
    std::string key, std::string value, const CommandControl& command_control) {
  auto shard = ShardByKey(key);
  return CreateRequest<RequestSetIfExist>(
      MakeRequest(CmdArgs{"set", std::move(key), std::move(value), "NX"}, shard,
                  true, GetCommandControl(command_control)));
}

RequestSetIfNotExist ClientImpl::SetIfNotExist(
    std::string key, std::string value, std::chrono::milliseconds ttl,
    const CommandControl& command_control) {
  auto shard = ShardByKey(key);
  return CreateRequest<RequestSetIfExist>(MakeRequest(
      CmdArgs{"set", std::move(key), std::move(value), "PX", ttl.count(), "NX"},
      shard, true, GetCommandControl(command_control)));
}

RequestSetex ClientImpl::Setex(std::string key, std::chrono::seconds seconds,
                               std::string value,
                               const CommandControl& command_control) {
  auto shard = ShardByKey(key);
  return CreateRequest<RequestSetex>(MakeRequest(
      CmdArgs{"setex", std::move(key), seconds.count(), std::move(value)},
      shard, true, GetCommandControl(command_control)));
}

RequestSismember ClientImpl::Sismember(std::string key, std::string member,
                                       const CommandControl& command_control) {
  auto shard = ShardByKey(key);
  return CreateRequest<RequestSismember>(
      MakeRequest(CmdArgs{"sismember", std::move(key), std::move(member)},
                  shard, false, GetCommandControl(command_control)));
}

RequestStrlen ClientImpl::Strlen(std::string key,
                                 const CommandControl& command_control) {
  auto shard = ShardByKey(key);
  return CreateRequest<RequestStrlen>(
      MakeRequest(CmdArgs{"strlen", std::move(key)}, shard, false,
                  GetCommandControl(command_control)));
}

RequestTtl ClientImpl::Ttl(std::string key,
                           const CommandControl& command_control) {
  auto shard = ShardByKey(key);
  return CreateRequest<RequestTtl>(
      MakeRequest(CmdArgs{"ttl", std::move(key)}, shard, false,
                  GetCommandControl(command_control)));
}

RequestType ClientImpl::Type(std::string key,
                             const CommandControl& command_control) {
  auto shard = ShardByKey(key);
  return CreateRequest<RequestType>(
      MakeRequest(CmdArgs{"type", std::move(key)}, shard, false,
                  GetCommandControl(command_control)));
}

RequestZadd ClientImpl::Zadd(std::string key, double score, std::string member,
                             const CommandControl& command_control) {
  auto shard = ShardByKey(key);
  return CreateRequest<RequestZadd>(
      MakeRequest(CmdArgs{"zadd", std::move(key), score, std::move(member)},
                  shard, true, GetCommandControl(command_control)));
}

RequestZadd ClientImpl::Zadd(std::string key, double score, std::string member,
                             const ZaddOptions& options,
                             const CommandControl& command_control) {
  auto shard = ShardByKey(key);
  return CreateRequest<RequestZadd>(MakeRequest(
      CmdArgs{"zadd", std::move(key), options, score, std::move(member)}, shard,
      true, GetCommandControl(command_control)));
}

RequestZaddIncr ClientImpl::ZaddIncr(std::string key, double score,
                                     std::string member,
                                     const CommandControl& command_control) {
  auto shard = ShardByKey(key);
  return CreateRequest<RequestZaddIncr>(MakeRequest(
      CmdArgs{"zadd", std::move(key), "INCR", score, std::move(member)}, shard,
      true, GetCommandControl(command_control)));
}

RequestZaddIncrExisting ClientImpl::ZaddIncrExisting(
    std::string key, double score, std::string member,
    const CommandControl& command_control) {
  auto shard = ShardByKey(key);
  return CreateRequest<RequestZaddIncrExisting>(MakeRequest(
      CmdArgs{"zadd", std::move(key), "XX", "INCR", score, std::move(member)},
      shard, true, GetCommandControl(command_control)));
}

RequestZcard ClientImpl::Zcard(std::string key,
                               const CommandControl& command_control) {
  auto shard = ShardByKey(key);
  return CreateRequest<RequestZcard>(
      MakeRequest(CmdArgs{"zcard", std::move(key)}, shard, false,
                  GetCommandControl(command_control)));
}

RequestZrangebyscore ClientImpl::Zrangebyscore(
    std::string key, double min, double max,
    const CommandControl& command_control) {
  auto shard = ShardByKey(key);
  return CreateRequest<RequestZrangebyscore>(
      MakeRequest(CmdArgs{"zrangebyscore", std::move(key), min, max}, shard,
                  false, GetCommandControl(command_control)));
}

RequestZrangebyscore ClientImpl::Zrangebyscore(
    std::string key, std::string min, std::string max,
    const CommandControl& command_control) {
  auto shard = ShardByKey(key);
  return CreateRequest<RequestZrangebyscore>(MakeRequest(
      CmdArgs{"zrangebyscore", std::move(key), std::move(min), std::move(max)},
      shard, false, GetCommandControl(command_control)));
}

RequestZrangebyscore ClientImpl::Zrangebyscore(
    std::string key, double min, double max, const RangeOptions& range_options,
    const CommandControl& command_control) {
  auto shard = ShardByKey(key);
  ::redis::RangeScoreOptions range_score_options{{false}, range_options};
  return CreateRequest<RequestZrangebyscore>(MakeRequest(
      CmdArgs{"zrangebyscore", std::move(key), min, max, range_score_options},
      shard, false, GetCommandControl(command_control)));
}

RequestZrangebyscore ClientImpl::Zrangebyscore(
    std::string key, std::string min, std::string max,
    const RangeOptions& range_options, const CommandControl& command_control) {
  auto shard = ShardByKey(key);
  ::redis::RangeScoreOptions range_score_options{{false}, range_options};
  return CreateRequest<RequestZrangebyscore>(
      MakeRequest(CmdArgs{"zrangebyscore", std::move(key), std::move(min),
                          std::move(max), range_score_options},
                  shard, false, GetCommandControl(command_control)));
}

RequestZrangebyscoreWithScores ClientImpl::ZrangebyscoreWithScores(
    std::string key, double min, double max,
    const CommandControl& command_control) {
  auto shard = ShardByKey(key);
  ::redis::RangeScoreOptions range_score_options{{true}, {}};
  return CreateRequest<RequestZrangebyscoreWithScores>(MakeRequest(
      CmdArgs{"zrangebyscore", std::move(key), min, max, range_score_options},
      shard, false, GetCommandControl(command_control)));
}

RequestZrangebyscoreWithScores ClientImpl::ZrangebyscoreWithScores(
    std::string key, std::string min, std::string max,
    const CommandControl& command_control) {
  auto shard = ShardByKey(key);
  ::redis::RangeScoreOptions range_score_options{{true}, {}};
  return CreateRequest<RequestZrangebyscoreWithScores>(
      MakeRequest(CmdArgs{"zrangebyscore", std::move(key), std::move(min),
                          std::move(max), range_score_options},
                  shard, false, GetCommandControl(command_control)));
}

RequestZrangebyscoreWithScores ClientImpl::ZrangebyscoreWithScores(
    std::string key, double min, double max, const RangeOptions& range_options,
    const CommandControl& command_control) {
  auto shard = ShardByKey(key);
  ::redis::RangeScoreOptions range_score_options{{true}, range_options};
  return CreateRequest<RequestZrangebyscoreWithScores>(MakeRequest(
      CmdArgs{"zrangebyscore", std::move(key), min, max, range_score_options},
      shard, false, GetCommandControl(command_control)));
}

RequestZrangebyscoreWithScores ClientImpl::ZrangebyscoreWithScores(
    std::string key, std::string min, std::string max,
    const RangeOptions& range_options, const CommandControl& command_control) {
  auto shard = ShardByKey(key);
  ::redis::RangeScoreOptions range_score_options{{true}, range_options};
  return CreateRequest<RequestZrangebyscoreWithScores>(
      MakeRequest(CmdArgs{"zrangebyscore", std::move(key), std::move(min),
                          std::move(max), range_score_options},
                  shard, false, GetCommandControl(command_control)));
}

RequestZrem ClientImpl::Zrem(std::string key, std::string member,
                             const CommandControl& command_control) {
  auto shard = ShardByKey(key);
  return CreateRequest<RequestZrem>(
      MakeRequest(CmdArgs{"zrem", std::move(key), std::move(member)}, shard,
                  true, GetCommandControl(command_control)));
}

RequestZrem ClientImpl::Zrem(std::string key, std::vector<std::string> members,
                             const CommandControl& command_control) {
  auto shard = ShardByKey(key);
  if (members.empty())
    return CreateDummyRequest<RequestZrem>(
        std::make_shared<::redis::Reply>("zrem", 0));
  return CreateRequest<RequestZrem>(
      MakeRequest(CmdArgs{"zrem", std::move(key), std::move(members)}, shard,
                  true, GetCommandControl(command_control)));
}

RequestZscore ClientImpl::Zscore(std::string key, std::string member,
                                 const CommandControl& command_control) {
  auto shard = ShardByKey(key);
  return CreateRequest<RequestZscore>(
      MakeRequest(CmdArgs{"zscore", std::move(key), std::move(member)}, shard,
                  false, GetCommandControl(command_control)));
}

::redis::Request ClientImpl::MakeRequest(
    CmdArgs&& args, size_t shard, bool master,
    const CommandControl& command_control) {
  return redis_client_->MakeRequest(std::move(args), shard, master,
                                    command_control);
}

CommandControl ClientImpl::GetCommandControl(const CommandControl& cc) const {
  return redis_client_->GetCommandControl(cc);
}

size_t ClientImpl::GetPublishShard(PubShard policy) {
  switch (policy) {
    case PubShard::kZeroShard:
      return 0;

    case PubShard::kRoundRobin:
      return ++publish_shard_ % ShardsCount();
  }

  return 0;
}

}  // namespace redis
}  // namespace storages
