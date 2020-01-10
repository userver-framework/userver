#include "client_impl.hpp"

#include <redis/sentinel.hpp>
#include <utils/assert.hpp>

#include "request_impl.hpp"
#include "transaction_impl.hpp"

namespace storages {
namespace redis {
namespace {

template <ScanTag>
const std::string kScanCommandName;

template <>
const std::string kScanCommandName<ScanTag::kScan> = "scan";
template <>
const std::string kScanCommandName<ScanTag::kSscan> = "sscan";
template <>
const std::string kScanCommandName<ScanTag::kHscan> = "hscan";
template <>
const std::string kScanCommandName<ScanTag::kZscan> = "zscan";

}  // namespace

ClientImpl::ClientImpl(std::shared_ptr<::redis::Sentinel> sentinel)
    : redis_client_(std::move(sentinel)) {}

size_t ClientImpl::ShardsCount() const { return redis_client_->ShardsCount(); }

size_t ClientImpl::ShardByKey(const std::string& key) const {
  return redis_client_->ShardByKey(key);
}

const std::string& ClientImpl::GetAnyKeyForShard(size_t shard_idx) const {
  return redis_client_->GetAnyKeyForShard(shard_idx);
}

Request<ScanReplyTmpl<ScanTag::kScan>> ClientImpl::MakeScanRequestNoKey(
    size_t shard, ScanReply::Cursor cursor, ScanOptions options,
    const CommandControl& command_control) {
  CmdArgs cmd_args{kScanCommandName<ScanTag::kScan>, cursor.GetValue(),
                   options.ExtractMatch(), options.ExtractCount()};
  return CreateRequest<Request<ScanReplyTmpl<ScanTag::kScan>>>(MakeRequest(
      std::move(cmd_args), shard, false, GetCommandControl(command_control)));
}

template <ScanTag scan_tag>
Request<ScanReplyTmpl<scan_tag>> ClientImpl::MakeScanRequestWithKey(
    std::string key, size_t shard,
    typename ScanReplyTmpl<scan_tag>::Cursor cursor,
    ScanOptionsTmpl<scan_tag> options, const CommandControl& command_control) {
  CmdArgs cmd_args{kScanCommandName<scan_tag>, std::move(key),
                   cursor.GetValue(), options.ExtractMatch(),
                   options.ExtractCount()};
  return CreateRequest<Request<ScanReplyTmpl<scan_tag>>>(MakeRequest(
      std::move(cmd_args), shard, false, GetCommandControl(command_control)));
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
    return CreateDummyRequest<RequestDel>(std::make_shared<Reply>("del", 0));
  auto shard = ShardByKey(keys.at(0));
  return CreateRequest<RequestDel>(
      MakeRequest(CmdArgs{"del", std::move(keys)}, shard, true,
                  GetCommandControl(command_control)));
}

RequestEvalCommon ClientImpl::EvalCommon(
    std::string script, std::vector<std::string> keys,
    std::vector<std::string> args, const CommandControl& command_control) {
  UASSERT(!keys.empty());
  auto shard = ShardByKey(keys.at(0));
  size_t keys_size = keys.size();
  return CreateRequest<RequestEvalCommon>(
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
        std::make_shared<Reply>("exists", 0));
  auto shard = ShardByKey(keys.at(0));
  return CreateRequest<RequestExists>(
      MakeRequest(CmdArgs{"exists", std::move(keys)}, shard, false,
                  GetCommandControl(command_control)));
}

RequestExpire ClientImpl::Expire(std::string key, std::chrono::seconds ttl,
                                 const CommandControl& command_control) {
  auto shard = ShardByKey(key);
  return CreateRequest<RequestExpire>(
      MakeRequest(CmdArgs{"expire", std::move(key), ttl.count()}, shard, true,
                  GetCommandControl(command_control)));
}

RequestGet ClientImpl::Get(std::string key,
                           const CommandControl& command_control) {
  auto shard = ShardByKey(key);
  return CreateRequest<RequestGet>(
      MakeRequest(CmdArgs{"get", std::move(key)}, shard, false,
                  GetCommandControl(command_control)));
}

RequestGetset ClientImpl::Getset(std::string key, std::string value,
                                 const CommandControl& command_control) {
  auto shard = ShardByKey(key);
  return CreateRequest<RequestGetset>(
      MakeRequest(CmdArgs{"getset", std::move(key), std::move(value)}, shard,
                  true, GetCommandControl(command_control)));
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
    return CreateDummyRequest<RequestHdel>(std::make_shared<Reply>("hdel", 0));
  auto shard = ShardByKey(key);
  return CreateRequest<RequestHdel>(
      MakeRequest(CmdArgs{"hdel", std::move(key), std::move(fields)}, shard,
                  true, GetCommandControl(command_control)));
}

RequestHexists ClientImpl::Hexists(std::string key, std::string field,
                                   const CommandControl& command_control) {
  auto shard = ShardByKey(key);
  return CreateRequest<RequestHexists>(
      MakeRequest(CmdArgs{"hexists", std::move(key), std::move(field)}, shard,
                  false, GetCommandControl(command_control)));
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

RequestHincrby ClientImpl::Hincrby(std::string key, std::string field,
                                   int64_t increment,
                                   const CommandControl& command_control) {
  auto shard = ShardByKey(key);
  return CreateRequest<RequestHincrby>(MakeRequest(
      CmdArgs{"hincrby", std::move(key), std::move(field), increment}, shard,
      true, GetCommandControl(command_control)));
}

RequestHincrbyfloat ClientImpl::Hincrbyfloat(
    std::string key, std::string field, double increment,
    const CommandControl& command_control) {
  auto shard = ShardByKey(key);
  return CreateRequest<RequestHincrbyfloat>(MakeRequest(
      CmdArgs{"hincrbyfloat", std::move(key), std::move(field), increment},
      shard, true, GetCommandControl(command_control)));
}

RequestHkeys ClientImpl::Hkeys(std::string key,
                               const CommandControl& command_control) {
  auto shard = ShardByKey(key);
  return CreateRequest<RequestHkeys>(
      MakeRequest(CmdArgs{"hkeys", std::move(key)}, shard, false,
                  GetCommandControl(command_control)));
}

RequestHlen ClientImpl::Hlen(std::string key,
                             const CommandControl& command_control) {
  auto shard = ShardByKey(key);
  return CreateRequest<RequestHlen>(
      MakeRequest(CmdArgs{"hlen", std::move(key)}, shard, false,
                  GetCommandControl(command_control)));
}

RequestHmget ClientImpl::Hmget(std::string key, std::vector<std::string> fields,
                               const CommandControl& command_control) {
  if (fields.empty())
    return CreateDummyRequest<RequestHmget>(
        std::make_shared<Reply>("hmget", ReplyData::Array{}));
  auto shard = ShardByKey(key);
  return CreateRequest<RequestHmget>(
      MakeRequest(CmdArgs{"hmget", std::move(key), std::move(fields)}, shard,
                  false, GetCommandControl(command_control)));
}

RequestHmset ClientImpl::Hmset(
    std::string key,
    std::vector<std::pair<std::string, std::string>> field_values,
    const CommandControl& command_control) {
  if (field_values.empty())
    return CreateDummyRequest<RequestHmset>(
        std::make_shared<Reply>("hmset", ReplyData::CreateStatus("OK")));
  auto shard = ShardByKey(key);
  return CreateRequest<RequestHmset>(
      MakeRequest(CmdArgs{"hmset", std::move(key), std::move(field_values)},
                  shard, true, GetCommandControl(command_control)));
}

ScanRequest<ScanTag::kHscan> ClientImpl::Hscan(
    std::string key, ScanOptionsTmpl<ScanTag::kHscan> options,
    const CommandControl& command_control) {
  return ScanTmpl<ScanTag::kHscan>(std::move(key), std::move(options),
                                   command_control);
}

RequestHset ClientImpl::Hset(std::string key, std::string field,
                             std::string value,
                             const CommandControl& command_control) {
  auto shard = ShardByKey(key);
  return CreateRequest<RequestHset>(MakeRequest(
      CmdArgs{"hset", std::move(key), std::move(field), std::move(value)},
      shard, true, GetCommandControl(command_control)));
}

RequestHsetnx ClientImpl::Hsetnx(std::string key, std::string field,
                                 std::string value,
                                 const CommandControl& command_control) {
  auto shard = ShardByKey(key);
  return CreateRequest<RequestHsetnx>(MakeRequest(
      CmdArgs{"hsetnx", std::move(key), std::move(field), std::move(value)},
      shard, true, GetCommandControl(command_control)));
}

RequestHvals ClientImpl::Hvals(std::string key,
                               const CommandControl& command_control) {
  auto shard = ShardByKey(key);
  return CreateRequest<RequestHvals>(
      MakeRequest(CmdArgs{"hvals", std::move(key)}, shard, false,
                  GetCommandControl(command_control)));
}

RequestIncr ClientImpl::Incr(std::string key,
                             const CommandControl& command_control) {
  auto shard = ShardByKey(key);
  return CreateRequest<RequestIncr>(
      MakeRequest(CmdArgs{"incr", std::move(key)}, shard, true,
                  GetCommandControl(command_control)));
}

RequestKeys ClientImpl::Keys(std::string keys_pattern, size_t shard,
                             const CommandControl& command_control) {
  return CreateRequest<RequestKeys>(
      MakeRequest(CmdArgs{"keys", std::move(keys_pattern)}, shard, false,
                  GetCommandControl(command_control)));
}

RequestLindex ClientImpl::Lindex(std::string key, int64_t index,
                                 const CommandControl& command_control) {
  auto shard = ShardByKey(key);
  return CreateRequest<RequestLindex>(
      MakeRequest(CmdArgs{"lindex", std::move(key), index}, shard, false,
                  GetCommandControl(command_control)));
}

RequestLlen ClientImpl::Llen(std::string key,
                             const CommandControl& command_control) {
  auto shard = ShardByKey(key);
  return CreateRequest<RequestLlen>(
      MakeRequest(CmdArgs{"llen", std::move(key)}, shard, false,
                  GetCommandControl(command_control)));
}

RequestLpop ClientImpl::Lpop(std::string key,
                             const CommandControl& command_control) {
  auto shard = ShardByKey(key);
  return CreateRequest<RequestLpop>(
      MakeRequest(CmdArgs{"lpop", std::move(key)}, shard, true,
                  GetCommandControl(command_control)));
}

RequestLpush ClientImpl::Lpush(std::string key, std::string value,
                               const CommandControl& command_control) {
  auto shard = ShardByKey(key);
  return CreateRequest<RequestLpush>(
      MakeRequest(CmdArgs{"lpush", std::move(key), std::move(value)}, shard,
                  true, GetCommandControl(command_control)));
}

RequestLpush ClientImpl::Lpush(std::string key, std::vector<std::string> values,
                               const CommandControl& command_control) {
  if (values.empty()) return Llen(std::move(key), command_control);
  auto shard = ShardByKey(key);
  return CreateRequest<RequestLpush>(
      MakeRequest(CmdArgs{"lpush", std::move(key), std::move(values)}, shard,
                  true, GetCommandControl(command_control)));
}

RequestLrange ClientImpl::Lrange(std::string key, int64_t start, int64_t stop,
                                 const CommandControl& command_control) {
  auto shard = ShardByKey(key);
  return CreateRequest<RequestLrange>(
      MakeRequest(CmdArgs{"lrange", std::move(key), start, stop}, shard, false,
                  GetCommandControl(command_control)));
}

RequestLrem ClientImpl::Lrem(std::string key, int64_t count,
                             std::string element,
                             const CommandControl& command_control) {
  auto shard = ShardByKey(key);
  return CreateRequest<RequestLrem>(
      MakeRequest(CmdArgs{"lrem", std::move(key), count, std::move(element)},
                  shard, true, GetCommandControl(command_control)));
}

RequestLtrim ClientImpl::Ltrim(std::string key, int64_t start, int64_t stop,
                               const CommandControl& command_control) {
  auto shard = ShardByKey(key);
  return CreateRequest<RequestLtrim>(
      MakeRequest(CmdArgs{"ltrim", std::move(key), start, stop}, shard, true,
                  GetCommandControl(command_control)));
}

RequestMget ClientImpl::Mget(std::vector<std::string> keys,
                             const CommandControl& command_control) {
  if (keys.empty())
    return CreateDummyRequest<RequestMget>(
        std::make_shared<Reply>("mget", ReplyData::Array{}));
  auto shard = ShardByKey(keys.at(0));
  return CreateRequest<RequestMget>(
      MakeRequest(CmdArgs{"mget", std::move(keys)}, shard, false,
                  GetCommandControl(command_control)));
}

RequestMset ClientImpl::Mset(
    std::vector<std::pair<std::string, std::string>> key_values,
    const CommandControl& command_control) {
  if (key_values.empty())
    return CreateDummyRequest<RequestMset>(std::make_shared<::redis::Reply>(
        "mset", ::redis::ReplyData::CreateStatus("OK")));
  auto shard = ShardByKey(key_values.at(0).first);
  return CreateRequest<RequestMset>(
      MakeRequest(CmdArgs{"mset", std::move(key_values)}, shard, true,
                  GetCommandControl(command_control)));
}

TransactionPtr ClientImpl::Multi() {
  return std::make_unique<TransactionImpl>(shared_from_this());
}

TransactionPtr ClientImpl::Multi(Transaction::CheckShards check_shards) {
  return std::make_unique<TransactionImpl>(shared_from_this(), check_shards);
}

RequestPersist ClientImpl::Persist(std::string key,
                                   const CommandControl& command_control) {
  auto shard = ShardByKey(key);
  return CreateRequest<RequestPersist>(
      MakeRequest(CmdArgs{"persist", std::move(key)}, shard, true,
                  GetCommandControl(command_control)));
}

RequestPexpire ClientImpl::Pexpire(std::string key,
                                   std::chrono::milliseconds ttl,
                                   const CommandControl& command_control) {
  auto shard = ShardByKey(key);
  return CreateRequest<RequestPexpire>(
      MakeRequest(CmdArgs{"pexpire", std::move(key), ttl.count()}, shard, true,
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
                         const CommandControl& command_control,
                         PubShard policy) {
  auto shard = GetPublishShard(policy);
  MakeRequest(CmdArgs{"publish", std::move(channel), std::move(message)}, shard,
              true, GetCommandControl(command_control));
}

RequestRename ClientImpl::Rename(std::string key, std::string new_key,
                                 const CommandControl& command_control) {
  auto shard = ShardByKey(key);
  auto new_shard = ShardByKey(new_key);
  if (shard != new_shard)
    throw ::redis::InvalidArgumentException(
        "shard of key != shard of new_key (" + std::to_string(shard) +
        " != " + std::to_string(new_shard) + ')');
  return CreateRequest<RequestRename>(
      MakeRequest(CmdArgs{"rename", std::move(key), std::move(new_key)}, shard,
                  true, GetCommandControl(command_control)));
}

RequestRpop ClientImpl::Rpop(std::string key,
                             const CommandControl& command_control) {
  auto shard = ShardByKey(key);
  return CreateRequest<RequestRpop>(
      MakeRequest(CmdArgs{"rpop", std::move(key)}, shard, true,
                  GetCommandControl(command_control)));
}

RequestRpush ClientImpl::Rpush(std::string key, std::string value,
                               const CommandControl& command_control) {
  auto shard = ShardByKey(key);
  return CreateRequest<RequestRpush>(
      MakeRequest(CmdArgs{"rpush", std::move(key), std::move(value)}, shard,
                  true, GetCommandControl(command_control)));
}

RequestRpush ClientImpl::Rpush(std::string key, std::vector<std::string> values,
                               const CommandControl& command_control) {
  if (values.empty()) return Llen(std::move(key), command_control);
  auto shard = ShardByKey(key);
  return CreateRequest<RequestRpush>(
      MakeRequest(CmdArgs{"rpush", std::move(key), std::move(values)}, shard,
                  true, GetCommandControl(command_control)));
}

RequestSadd ClientImpl::Sadd(std::string key, std::string member,
                             const CommandControl& command_control) {
  auto shard = ShardByKey(key);
  return CreateRequest<RequestSadd>(
      MakeRequest(CmdArgs{"sadd", std::move(key), std::move(member)}, shard,
                  true, GetCommandControl(command_control)));
}

RequestSadd ClientImpl::Sadd(std::string key, std::vector<std::string> members,
                             const CommandControl& command_control) {
  if (members.empty())
    return CreateDummyRequest<RequestSadd>(std::make_shared<Reply>("sadd", 0));
  auto shard = ShardByKey(key);
  return CreateRequest<RequestSadd>(
      MakeRequest(CmdArgs{"sadd", std::move(key), std::move(members)}, shard,
                  true, GetCommandControl(command_control)));
}

ScanRequest<ScanTag::kScan> ClientImpl::Scan(
    size_t shard, ScanOptionsTmpl<ScanTag::kScan> options,
    const CommandControl& command_control) {
  // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDeleteLeaks)
  return ScanRequest<ScanTag::kScan>(
      std::make_unique<RequestScanData<ScanTag::kScan>>(
          shared_from_this(), shard, std::move(options), command_control));
}

template <ScanTag scan_tag>
ScanRequest<scan_tag> ClientImpl::ScanTmpl(
    std::string key, ScanOptionsTmpl<scan_tag> options,
    const CommandControl& command_control) {
  auto shard = ShardByKey(key);
  // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDeleteLeaks)
  return ScanRequest<scan_tag>(std::make_unique<RequestScanData<scan_tag>>(
      shared_from_this(), std::move(key), shard, std::move(options),
      command_control));
}

RequestScard ClientImpl::Scard(std::string key,
                               const CommandControl& command_control) {
  auto shard = ShardByKey(key);
  return CreateRequest<RequestScard>(
      MakeRequest(CmdArgs{"scard", std::move(key)}, shard, false,
                  GetCommandControl(command_control)));
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

RequestSmembers ClientImpl::Smembers(std::string key,
                                     const CommandControl& command_control) {
  auto shard = ShardByKey(key);
  return CreateRequest<RequestSmembers>(
      MakeRequest(CmdArgs{"smembers", std::move(key)}, shard, false,
                  GetCommandControl(command_control)));
}

RequestSrandmember ClientImpl::Srandmember(
    std::string key, const CommandControl& command_control) {
  auto shard = ShardByKey(key);
  return CreateRequest<RequestSrandmember>(
      MakeRequest(CmdArgs{"srandmember", std::move(key)}, shard, false,
                  GetCommandControl(command_control)));
}

RequestSrandmembers ClientImpl::Srandmembers(
    std::string key, int64_t count, const CommandControl& command_control) {
  auto shard = ShardByKey(key);
  return CreateRequest<RequestSrandmembers>(
      MakeRequest(CmdArgs{"srandmember", std::move(key), count}, shard, false,
                  GetCommandControl(command_control)));
}

RequestSrem ClientImpl::Srem(std::string key, std::string member,
                             const CommandControl& command_control) {
  auto shard = ShardByKey(key);
  return CreateRequest<RequestSrem>(
      MakeRequest(CmdArgs{"srem", std::move(key), std::move(member)}, shard,
                  true, GetCommandControl(command_control)));
}

RequestSrem ClientImpl::Srem(std::string key, std::vector<std::string> members,
                             const CommandControl& command_control) {
  if (members.empty())
    return CreateDummyRequest<RequestSrem>(std::make_shared<Reply>("srem", 0));
  auto shard = ShardByKey(key);
  return CreateRequest<RequestSrem>(
      MakeRequest(CmdArgs{"srem", std::move(key), std::move(members)}, shard,
                  true, GetCommandControl(command_control)));
}

ScanRequest<ScanTag::kSscan> ClientImpl::Sscan(
    std::string key, ScanOptionsTmpl<ScanTag::kSscan> options,
    const CommandControl& command_control) {
  return ScanTmpl<ScanTag::kSscan>(std::move(key), std::move(options),
                                   command_control);
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
    return CreateDummyRequest<RequestZrem>(std::make_shared<Reply>("zrem", 0));
  return CreateRequest<RequestZrem>(
      MakeRequest(CmdArgs{"zrem", std::move(key), std::move(members)}, shard,
                  true, GetCommandControl(command_control)));
}

ScanRequest<ScanTag::kZscan> ClientImpl::Zscan(
    std::string key, ScanOptionsTmpl<ScanTag::kZscan> options,
    const CommandControl& command_control) {
  return ScanTmpl<ScanTag::kZscan>(std::move(key), std::move(options),
                                   command_control);
}

RequestZscore ClientImpl::Zscore(std::string key, std::string member,
                                 const CommandControl& command_control) {
  auto shard = ShardByKey(key);
  return CreateRequest<RequestZscore>(
      MakeRequest(CmdArgs{"zscore", std::move(key), std::move(member)}, shard,
                  false, GetCommandControl(command_control)));
}

// end of redis commands

::redis::Request ClientImpl::MakeRequest(CmdArgs&& args, size_t shard,
                                         bool master,
                                         const CommandControl& command_control,
                                         bool skip_status) {
  return redis_client_->MakeRequest(std::move(args), shard, master,
                                    command_control, skip_status);
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

template Request<ScanReplyTmpl<ScanTag::kSscan>>
ClientImpl::MakeScanRequestWithKey(
    std::string key, size_t shard,
    ScanReplyTmpl<ScanTag::kSscan>::Cursor cursor,
    ScanOptionsTmpl<ScanTag::kSscan> options,
    const CommandControl& command_control);

template Request<ScanReplyTmpl<ScanTag::kHscan>>
ClientImpl::MakeScanRequestWithKey(
    std::string key, size_t shard,
    ScanReplyTmpl<ScanTag::kHscan>::Cursor cursor,
    ScanOptionsTmpl<ScanTag::kHscan> options,
    const CommandControl& command_control);

template Request<ScanReplyTmpl<ScanTag::kZscan>>
ClientImpl::MakeScanRequestWithKey(
    std::string key, size_t shard,
    ScanReplyTmpl<ScanTag::kZscan>::Cursor cursor,
    ScanOptionsTmpl<ScanTag::kZscan> options,
    const CommandControl& command_control);

}  // namespace redis
}  // namespace storages
