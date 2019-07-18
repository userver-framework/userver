#include <storages/redis/client_impl.hpp>

#include <redis/sentinel.hpp>
#include <utils/assert.hpp>

#include <storages/redis/request_impl.hpp>

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

RequestAppend ClientImpl::Append(const std::string& key, std::string value,
                                 const CommandControl& command_control) {
  return CreateRequest<RequestAppend>(
      MakeRequest(CmdArgs{"append", key, std::move(value)}, key, true,
                  GetCommandControl(command_control)));
}

RequestDel ClientImpl::Del(const std::string& key,
                           const CommandControl& command_control) {
  return CreateRequest<RequestDel>(MakeRequest(
      CmdArgs{"del", key}, key, true, GetCommandControl(command_control)));
}

RequestGet ClientImpl::Get(const std::string& key,
                           const CommandControl& command_control) {
  return CreateRequest<RequestGet>(MakeRequest(
      CmdArgs{"get", key}, key, false, GetCommandControl(command_control)));
}

RequestHdel ClientImpl::Hdel(const std::string& key, const std::string& field,
                             const CommandControl& command_control) {
  return CreateRequest<RequestHdel>(
      MakeRequest(CmdArgs{"hdel", key, field}, key, true,
                  GetCommandControl(command_control)));
}

RequestHget ClientImpl::Hget(const std::string& key, const std::string& field,
                             const CommandControl& command_control) {
  return CreateRequest<RequestHget>(
      MakeRequest(CmdArgs{"hget", key, field}, key, false,
                  GetCommandControl(command_control)));
}

RequestHgetall ClientImpl::Hgetall(const std::string& key,
                                   const CommandControl& command_control) {
  return CreateRequest<RequestHgetall>(MakeRequest(
      CmdArgs{"hgetall", key}, key, false, GetCommandControl(command_control)));
}

RequestHmset ClientImpl::Hmset(
    const std::string& key,
    const std::vector<std::pair<std::string, std::string>>& field_val,
    const CommandControl& command_control) {
  return CreateRequest<RequestHmset>(
      MakeRequest(CmdArgs{"hmset", key, field_val}, key, true,
                  GetCommandControl(command_control)));
}

RequestKeys ClientImpl::Keys(std::string keys_pattern, size_t shard,
                             const CommandControl& command_control) {
  return CreateRequest<RequestKeys>(
      MakeRequest(CmdArgs{"keys", std::move(keys_pattern)}, shard, false,
                  GetCommandControl(command_control)));
}

RequestHset ClientImpl::Hset(const std::string& key, const std::string& field,
                             const std::string& value,
                             const CommandControl& command_control) {
  return CreateRequest<RequestHset>(
      MakeRequest(CmdArgs{"hset", key, field, value}, key, true,
                  GetCommandControl(command_control)));
}

RequestPublish ClientImpl::Publish(const std::string& channel,
                                   std::string message,
                                   const CommandControl& command_control) {
  return Publish(channel, std::move(message), command_control,
                 PubShard::kZeroShard);
}

RequestPublish ClientImpl::Publish(const std::string& channel,
                                   std::string message,
                                   const CommandControl& command_control,
                                   PubShard policy) {
  auto shard = GetPublishShard(policy);
  return CreateRequest<RequestPublish>(
      MakeRequest(CmdArgs{"publish", channel, std::move(message)}, shard, false,
                  GetCommandControl(command_control)));
}

RequestMget ClientImpl::Mget(std::vector<std::string> keys,
                             const CommandControl& command_control) {
  UASSERT(!keys.empty());
  auto key0 = keys.at(0);
  return CreateRequest<RequestMget>(
      MakeRequest(CmdArgs{"mget", std::move(keys)}, key0, false,
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

RequestSet ClientImpl::Set(const std::string& key, std::string value,
                           const CommandControl& command_control) {
  return CreateRequest<RequestSet>(
      MakeRequest(CmdArgs{"set", key, std::move(value)}, key, true,
                  GetCommandControl(command_control)));
}

RequestSetex ClientImpl::Setex(const std::string& key, int64_t seconds,
                               std::string value,
                               const CommandControl& command_control) {
  return CreateRequest<RequestSetex>(
      MakeRequest(CmdArgs{"setex", key, seconds, std::move(value)}, key, true,
                  GetCommandControl(command_control)));
}

RequestStrlen ClientImpl::Strlen(const std::string& key,
                                 const CommandControl& command_control) {
  return CreateRequest<RequestStrlen>(MakeRequest(
      CmdArgs{"strlen", key}, key, false, GetCommandControl(command_control)));
}

RequestTtl ClientImpl::Ttl(const std::string& key,
                           const CommandControl& command_control) {
  return CreateRequest<RequestTtl>(MakeRequest(
      CmdArgs{"ttl", key}, key, false, GetCommandControl(command_control)));
}

RequestType ClientImpl::Type(const std::string& key,
                             const CommandControl& command_control) {
  return CreateRequest<RequestType>(MakeRequest(
      CmdArgs{"type", key}, key, false, GetCommandControl(command_control)));
}

RequestZadd ClientImpl::Zadd(const std::string& key, double score,
                             const std::string& member,
                             const CommandControl& command_control) {
  return CreateRequest<RequestZadd>(
      MakeRequest(CmdArgs{"zadd", key, score, member}, key, true,
                  GetCommandControl(command_control)));
}

RequestZadd ClientImpl::Zadd(const std::string& key, double score,
                             const std::string& member,
                             const ZaddOptions& options,
                             const CommandControl& command_control) {
  return CreateRequest<RequestZadd>(
      MakeRequest(CmdArgs{"zadd", key, options, score, member}, key, true,
                  GetCommandControl(command_control)));
}

RequestZrangebyscore ClientImpl::Zrangebyscore(
    const std::string& key, double min, double max,
    const RangeOptions& range_options, const CommandControl& command_control) {
  ::redis::RangeScoreOptions range_score_options{{false}, range_options};
  return CreateRequest<RequestZrangebyscore>(
      MakeRequest(CmdArgs{"zrangebyscore", key, min, max, range_score_options},
                  key, false, GetCommandControl(command_control)));
}

RequestZrangebyscore ClientImpl::Zrangebyscore(
    const std::string& key, const std::string& min, const std::string& max,
    const RangeOptions& range_options, const CommandControl& command_control) {
  ::redis::RangeScoreOptions range_score_options{{false}, range_options};
  return CreateRequest<RequestZrangebyscore>(
      MakeRequest(CmdArgs{"zrangebyscore", key, min, max, range_score_options},
                  key, false, GetCommandControl(command_control)));
}

RequestZrangebyscoreWithScores ClientImpl::ZrangebyscoreWithScores(
    const std::string& key, double min, double max,
    const RangeOptions& range_options, const CommandControl& command_control) {
  ::redis::RangeScoreOptions range_score_options{{true}, range_options};
  return CreateRequest<RequestZrangebyscoreWithScores>(
      MakeRequest(CmdArgs{"zrangebyscore", key, min, max, range_score_options},
                  key, false, GetCommandControl(command_control)));
}

RequestZrangebyscoreWithScores ClientImpl::ZrangebyscoreWithScores(
    const std::string& key, const std::string& min, const std::string& max,
    const RangeOptions& range_options, const CommandControl& command_control) {
  ::redis::RangeScoreOptions range_score_options{{true}, range_options};
  return CreateRequest<RequestZrangebyscoreWithScores>(
      MakeRequest(CmdArgs{"zrangebyscore", key, min, max, range_score_options},
                  key, false, GetCommandControl(command_control)));
}

RequestZrem ClientImpl::Zrem(const std::string& key, const std::string& member,
                             const CommandControl& command_control) {
  return CreateRequest<RequestZrem>(
      MakeRequest(CmdArgs{"zrem", key, member}, key, true,
                  GetCommandControl(command_control)));
}

RequestZscore ClientImpl::Zscore(const std::string& key,
                                 const std::string& member,
                                 const CommandControl& command_control) {
  return CreateRequest<RequestZscore>(
      MakeRequest(CmdArgs{"zscore", key, member}, key, false,
                  GetCommandControl(command_control)));
}

::redis::Request ClientImpl::MakeRequest(
    CmdArgs&& args, const std::string& key, bool master,
    const CommandControl& command_control) {
  return redis_client_->MakeRequest(std::move(args), key, master,
                                    command_control);
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
