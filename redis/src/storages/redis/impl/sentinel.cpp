#include <storages/redis/impl/sentinel.hpp>

#include <memory>
#include <stdexcept>

#include <logging/log.hpp>

#include <engine/ev/thread_control.hpp>
#include <engine/task/cancel.hpp>
#include <engine/task/task_context.hpp>
#include <utils/assert.hpp>

#include <storages/redis/impl/exception.hpp>
#include <storages/redis/impl/reply.hpp>
#include "redis.hpp"
#include "sentinel_impl.hpp"
#include "subscribe_sentinel.hpp"

namespace redis {
namespace {

void ThrowIfCancelled() {
  if (engine::current_task::GetCurrentTaskContextUnchecked() &&
      engine::current_task::ShouldCancel()) {
    throw RequestCancelledException(
        "Failed to make redis request due to task cancellation");
  }
}

}  // namespace

Sentinel::Sentinel(const std::shared_ptr<ThreadPools>& thread_pools,
                   const std::vector<std::string>& shards,
                   const std::vector<ConnectionInfo>& conns,
                   std::string shard_group_name, const std::string& client_name,
                   const std::string& password,
                   ReadyChangeCallback ready_callback,
                   std::unique_ptr<KeyShard>&& key_shard,
                   CommandControl command_control, bool track_masters,
                   bool track_slaves)
    : thread_pools_(thread_pools),
      secdist_default_command_control_(command_control) {
  config_default_command_control_.Set(
      std::make_shared<CommandControl>(secdist_default_command_control_));

  if (!thread_pools_) {
    throw std::runtime_error("can't create Sentinel with empty thread_pools");
  }
  sentinel_thread_control_ = std::make_unique<engine::ev::ThreadControl>(
      thread_pools_->GetSentinelThreadPool().NextThread());

  auto ready_callback_2 = [this, ready_callback](size_t shard,
                                                 const std::string& shard_name,
                                                 bool master, bool ready) {
    if (ready) OnConnectionReady(shard, shard_name, master);
    ready_callback(shard, shard_name, master, ready);
  };

  sentinel_thread_control_->RunInEvLoopBlocking([&]() {
    impl_ = std::make_unique<SentinelImpl>(
        *sentinel_thread_control_, thread_pools_->GetRedisThreadPool(), *this,
        shards, conns, std::move(shard_group_name), client_name, password,
        std::move(ready_callback_2), std::move(key_shard), track_masters,
        track_slaves);
  });
}

Sentinel::~Sentinel() {
  sentinel_thread_control_->RunInEvLoopBlocking([this]() { impl_.reset(); });
  UASSERT(!impl_);
}

void Sentinel::WaitConnectedDebug() { impl_->WaitConnectedDebug(); }

void Sentinel::ForceUpdateHosts() { impl_->ForceUpdateHosts(); }

std::shared_ptr<Sentinel> Sentinel::CreateSentinel(
    const std::shared_ptr<ThreadPools>& thread_pools,
    const secdist::RedisSettings& settings, std::string shard_group_name,
    const std::string& client_name, KeyShardFactory key_shard_factory) {
  auto ready_callback = [](size_t shard, const std::string& shard_name,
                           bool master, bool ready) {
    LOG_INFO() << "redis: ready_callback:"
               << "  shard = " << shard << "  shard_name = " << shard_name
               << "  master = " << (master ? "true" : "false")
               << "  ready = " << (ready ? "true" : "false");
  };
  return CreateSentinel(thread_pools, settings, std::move(shard_group_name),
                        client_name, std::move(ready_callback),
                        std::move(key_shard_factory));
}

std::shared_ptr<Sentinel> Sentinel::CreateSentinel(
    const std::shared_ptr<ThreadPools>& thread_pools,
    const secdist::RedisSettings& settings, std::string shard_group_name,
    const std::string& client_name,
    Sentinel::ReadyChangeCallback ready_callback,
    KeyShardFactory key_shard_factory) {
  const std::string& password = settings.password;

  const std::vector<std::string>& shards = settings.shards;
  LOG_DEBUG() << "shards.size() = " << shards.size();
  for (const std::string& shard : shards)
    LOG_DEBUG() << "shard:  name = " << shard;

  std::vector<redis::ConnectionInfo> conns;
  conns.reserve(settings.sentinels.size());
  LOG_DEBUG() << "sentinels.size() = " << settings.sentinels.size();
  for (const auto& sentinel : settings.sentinels) {
    LOG_DEBUG() << "sentinel:  host = " << sentinel.host
                << "  port = " << sentinel.port;
    conns.emplace_back(sentinel.host, sentinel.port, "");
  }
  redis::CommandControl command_control = redis::command_control_init;
  LOG_DEBUG() << "redis command_control:"
              << "  timeout_single = " << command_control.timeout_single.count()
              << "ms"
              << "  timeout_all = " << command_control.timeout_all.count()
              << "ms"
              << "  max_retries = " << command_control.max_retries;
  std::shared_ptr<redis::Sentinel> client;
  if (!shards.empty() && !conns.empty()) {
    client = std::make_shared<redis::Sentinel>(
        thread_pools, shards, conns, std::move(shard_group_name), client_name,
        password, std::move(ready_callback), key_shard_factory(shards.size()),
        command_control, true, true);
  }

  return client;
}

void Sentinel::Restart() {
  sentinel_thread_control_->RunInEvLoopBlocking([&]() {
    impl_->Stop();
    impl_->Init();
    impl_->Start();
  });
}

std::unordered_map<ServerId, size_t, ServerIdHasher>
Sentinel::GetAvailableServersWeighted(size_t shard_idx, bool with_master,
                                      const CommandControl& cc) const {
  return impl_->GetAvailableServersWeighted(shard_idx, with_master,
                                            GetCommandControl(cc));
}

void Sentinel::AsyncCommand(CommandPtr command, bool master, size_t shard) {
  if (!impl_) return;
  ThrowIfCancelled();
  if (command->control.force_request_to_master) master = true;
  if (command->control.force_shard_idx) {
    if (shard != *command->control.force_shard_idx)
      throw InvalidArgumentException(
          "shard index in argument differs from force_shard_idx in "
          "command_control (" +
          std::to_string(shard) +
          " != " + std::to_string(*command->control.force_shard_idx) + ')');
  }
  CheckShardIdx(shard);
  try {
    impl_->AsyncCommand(
        {command, master, shard, std::chrono::steady_clock::now()});
  } catch (const std::exception& ex) {
    LOG_WARNING() << "exception in " << __func__ << " '" << ex.what() << "'";
  }
}

void Sentinel::AsyncCommand(CommandPtr command, const std::string& key,
                            bool master) {
  if (!impl_) return;
  ThrowIfCancelled();
  if (command->control.force_request_to_master) master = true;
  size_t shard = command->control.force_shard_idx
                     ? *command->control.force_shard_idx
                     : impl_->ShardByKey(key);
  CheckShardIdx(shard);
  try {
    impl_->AsyncCommand(
        {command, master, shard, std::chrono::steady_clock::now()});
  } catch (const std::exception& ex) {
    LOG_WARNING() << "exception in " << __func__ << " '" << ex.what() << "'";
  }
}

std::string Sentinel::CreateTmpKey(const std::string& key, std::string prefix) {
  size_t key_start, key_len;
  GetRedisKey(key, key_start, key_len);

  std::string tmp_key{std::move(prefix)};
  if (key_start == 0) {
    tmp_key.push_back('{');
    tmp_key.append(key);
    tmp_key.push_back('}');
  } else {
    tmp_key.append(key);
  }
  return tmp_key;
}

size_t Sentinel::ShardByKey(const std::string& key) const {
  return impl_->ShardByKey(key);
}

size_t Sentinel::ShardsCount() const { return impl_->ShardsCount(); }

void Sentinel::CheckShardIdx(size_t shard_idx) const {
  CheckShardIdx(shard_idx, ShardsCount());
}

void Sentinel::CheckShardIdx(size_t shard_idx, size_t shard_count) {
  if (shard_idx >= shard_count)
    throw InvalidArgumentException("invalid shard (" +
                                   std::to_string(shard_idx) +
                                   " >= " + std::to_string(shard_count) + ')');
}

const std::string& Sentinel::GetAnyKeyForShard(size_t shard_idx) const {
  return impl_->GetAnyKeyForShard(shard_idx);
}

SentinelStatistics Sentinel::GetStatistics() const {
  return impl_->GetStatistics();
}

std::vector<Request> Sentinel::MakeRequests(
    CmdArgs&& args, bool master, const CommandControl& command_control,
    bool skip_status) {
  std::vector<Request> rslt;

  for (size_t shard = 0; shard < impl_->ShardsCount(); ++shard) {
    rslt.push_back(
        MakeRequest(args.Clone(), shard, master, command_control, skip_status));
  }

  return rslt;
}

Request Sentinel::Append(const std::string& key, const std::string& value,
                         const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"append", key, value}, key, true,
                     GetCommandControl(command_control));
}

Request Sentinel::Bitcount(const std::string& key,
                           const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"bitcount", key}, key, false,
                     GetCommandControl(command_control));
}

Request Sentinel::Bitcount(const std::string& key, int64_t start, int64_t end,
                           const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"bitcount", key, start, end}, key, false,
                     GetCommandControl(command_control));
}

Request Sentinel::Bitop(const std::string& operation,
                        const std::string& destkey, const std::string& key,
                        const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"bitop", operation, destkey, key}, key, true,
                     GetCommandControl(command_control));
}

Request Sentinel::Bitop(const std::string& operation,
                        const std::string& destkey,
                        const std::vector<std::string>& keys,
                        const CommandControl& command_control) {
  UASSERT(!keys.empty());
  return MakeRequest(CmdArgs{"bitop", operation, destkey, keys}, keys[0], true,
                     GetCommandControl(command_control));
}

Request Sentinel::Bitpos(const std::string& key, int bit,
                         const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"bitpos", key, bit}, key, false,
                     GetCommandControl(command_control));
}

Request Sentinel::Bitpos(const std::string& key, int bit, int64_t start,
                         const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"bitpos", key, bit, start}, key, false,
                     GetCommandControl(command_control));
}

Request Sentinel::Bitpos(const std::string& key, int bit, int64_t start,
                         int64_t end, const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"bitpos", key, bit, start, end}, key, false,
                     GetCommandControl(command_control));
}

Request Sentinel::Blpop(const std::string& key, int64_t timeout,
                        const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"blpop", key, timeout}, key, true,
                     GetCommandControl(command_control));
}

Request Sentinel::Blpop(const std::vector<std::string>& keys, int64_t timeout,
                        const CommandControl& command_control) {
  UASSERT(!keys.empty());
  return MakeRequest(CmdArgs{"blpop", keys, timeout}, keys[0], true,
                     GetCommandControl(command_control));
}

Request Sentinel::Brpop(const std::string& key, int64_t timeout,
                        const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"brpop", key, timeout}, key, true,
                     GetCommandControl(command_control));
}

Request Sentinel::Brpop(const std::vector<std::string>& keys, int64_t timeout,
                        const CommandControl& command_control) {
  UASSERT(!keys.empty());
  return MakeRequest(CmdArgs{"brpop", keys, timeout}, keys[0], true,
                     GetCommandControl(command_control));
}

Request Sentinel::DbsizeEstimated(size_t shard,
                                  const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"dbsize"}, shard, false,
                     GetCommandControl(command_control));
}

Request Sentinel::Decr(const std::string& key,
                       const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"decr", key}, key, true,
                     GetCommandControl(command_control));
}

Request Sentinel::Decrby(const std::string& key, int64_t val,
                         const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"decrby", key, val}, key, true,
                     GetCommandControl(command_control));
}

Request Sentinel::Del(const std::string& key,
                      const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"del", key}, key, true,
                     GetCommandControl(command_control));
}

Request Sentinel::Del(const std::vector<std::string>& keys,
                      const CommandControl& command_control) {
  UASSERT(!keys.empty());
  return MakeRequest(CmdArgs{"del", keys}, keys[0], true,
                     GetCommandControl(command_control));
}

Request Sentinel::Dump(const std::string& key,
                       const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"dump", key}, key, false,
                     GetCommandControl(command_control));
}

Request Sentinel::Eval(const std::string& script,
                       const std::vector<std::string>& keys,
                       const std::vector<std::string>& args,
                       const CommandControl& command_control) {
  UASSERT(!keys.empty());
  return MakeRequest(CmdArgs{"eval", script, keys.size(), keys, args}, keys[0],
                     true, GetCommandControl(command_control));
}

Request Sentinel::Evalsha(const std::string& sha1,
                          const std::vector<std::string>& keys,
                          const std::vector<std::string>& args,
                          const CommandControl& command_control) {
  UASSERT(!keys.empty());
  return MakeRequest(CmdArgs{"evalsha", sha1, keys.size(), keys, args}, keys[0],
                     true, GetCommandControl(command_control));
}

Request Sentinel::Exists(const std::string& key,
                         const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"exists", key}, key, false,
                     GetCommandControl(command_control));
}

Request Sentinel::Exists(const std::vector<std::string>& keys,
                         const CommandControl& command_control) {
  UASSERT(!keys.empty());
  return MakeRequest(CmdArgs{"exists", keys}, keys[0], false,
                     GetCommandControl(command_control));
}

Request Sentinel::Expire(const std::string& key, int64_t seconds,
                         const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"expire", key, seconds}, key, true,
                     GetCommandControl(command_control));
}

Request Sentinel::Expireat(const std::string& key, int64_t timestamp,
                           const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"expireat", key, timestamp}, key, true,
                     GetCommandControl(command_control));
}

std::vector<Request> Sentinel::FlushAll(const CommandControl& command_control) {
  return MakeRequests(CmdArgs{"flushall"}, true,
                      GetCommandControl(command_control));
}

Request Sentinel::Geoadd(const std::string& key,
                         const GeoaddArg& lon_lat_member,
                         const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"geoadd", key, lon_lat_member}, key, true,
                     GetCommandControl(command_control));
}

Request Sentinel::Geoadd(const std::string& key,
                         const std::vector<GeoaddArg>& lon_lat_member,
                         const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"geoadd", key, lon_lat_member}, key, true,
                     GetCommandControl(command_control));
}

Request Sentinel::Geodist(const std::string& key, const std::string& member_1,
                          const std::string& member_2,
                          const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"geodist", key, member_1, member_2}, key, false,
                     GetCommandControl(command_control));
}

Request Sentinel::Geodist(const std::string& key, const std::string& member_1,
                          const std::string& member_2, const std::string& unit,
                          const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"geodist", key, member_1, member_2, unit}, key,
                     false, GetCommandControl(command_control));
}

Request Sentinel::Geohash(const std::string& key, const std::string& member,
                          const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"geohash", key, member}, key, false,
                     GetCommandControl(command_control));
}

Request Sentinel::Geohash(const std::string& key,
                          const std::vector<std::string>& members,
                          const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"geohash", key, members}, key, false,
                     GetCommandControl(command_control));
}

Request Sentinel::Geopos(const std::string& key, const std::string& member,
                         const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"geopos", key, member}, key, false,
                     GetCommandControl(command_control));
}

Request Sentinel::Geopos(const std::string& key,
                         const std::vector<std::string>& members,
                         const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"geopos", key, members}, key, false,
                     GetCommandControl(command_control));
}

Request Sentinel::Georadius(const std::string& key, double lon, double lat,
                            double radius, const GeoradiusOptions& options,
                            const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"georadius", key, lon, lat, radius, options}, key,
                     false, GetCommandControl(command_control));
}

Request Sentinel::Georadius(const std::string& key, double lon, double lat,
                            double radius, const std::string& unit,
                            const GeoradiusOptions& options,
                            const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"georadius", key, lon, lat, radius, unit, options},
                     key, false, GetCommandControl(command_control));
}

Request Sentinel::Georadiusbymember(const std::string& key,
                                    const std::string& member, double radius,
                                    const GeoradiusOptions& options,
                                    const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"georadiusbymember", key, member, radius, options},
                     key, false, GetCommandControl(command_control));
}

Request Sentinel::Georadiusbymember(const std::string& key,
                                    const std::string& member, double radius,
                                    const std::string& unit,
                                    const GeoradiusOptions& options,
                                    const CommandControl& command_control) {
  return MakeRequest(
      CmdArgs{"georadiusbymember", key, member, radius, unit, options}, key,
      false, GetCommandControl(command_control));
}

Request Sentinel::Get(const std::string& key,
                      const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"get", key}, key, false,
                     GetCommandControl(command_control));
}

Request Sentinel::Get(const std::string& key, RetryNilFromMaster,
                      const CommandControl& command_control) {
  return Get(key, command_control.MergeWith(kRetryNilFromMaster));
}

Request Sentinel::Getbit(const std::string& key, int64_t offset,
                         const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"getbit", key, offset}, key, false,
                     GetCommandControl(command_control));
}

Request Sentinel::Getrange(const std::string& key, int64_t start, int64_t end,
                           const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"getrange", key, start, end}, key, false,
                     GetCommandControl(command_control));
}

Request Sentinel::Getset(const std::string& key, const std::string& val,
                         const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"getset", key, val}, key, true,
                     GetCommandControl(command_control));
}

Request Sentinel::Hdel(const std::string& key, const std::string& field,
                       const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"hdel", key, field}, key, true,
                     GetCommandControl(command_control));
}

Request Sentinel::Hdel(const std::string& key,
                       const std::vector<std::string>& fields,
                       const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"hdel", key, fields}, key, true,
                     GetCommandControl(command_control));
}

Request Sentinel::Hexists(const std::string& key, const std::string& field,
                          const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"hexists", key, field}, key, false,
                     GetCommandControl(command_control));
}

Request Sentinel::Hget(const std::string& key, const std::string& field,
                       const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"hget", key, field}, key, false,
                     GetCommandControl(command_control));
}

Request Sentinel::Hgetall(const std::string& key,
                          const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"hgetall", key}, key, false,
                     GetCommandControl(command_control));
}

Request Sentinel::Hincrby(const std::string& key, const std::string& field,
                          int64_t incr, const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"hincrby", key, field, incr}, key, true,
                     GetCommandControl(command_control));
}

Request Sentinel::Hincrbyfloat(const std::string& key, const std::string& field,
                               double incr,
                               const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"hincrbyfloat", key, field, incr}, key, true,
                     GetCommandControl(command_control));
}

Request Sentinel::Hkeys(const std::string& key,
                        const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"hkeys", key}, key, false,
                     GetCommandControl(command_control));
}

Request Sentinel::Hlen(const std::string& key,
                       const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"hlen", key}, key, false,
                     GetCommandControl(command_control));
}

Request Sentinel::Hmget(const std::string& key,
                        const std::vector<std::string>& fields,
                        const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"hmget", key, fields}, key, false,
                     GetCommandControl(command_control));
}

Request Sentinel::Hmset(
    const std::string& key,
    const std::vector<std::pair<std::string, std::string>>& field_val,
    const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"hmset", key, field_val}, key, true,
                     GetCommandControl(command_control));
}

Request Sentinel::Hset(const std::string& key, const std::string& field,
                       const std::string& value,
                       const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"hset", key, field, value}, key, true,
                     GetCommandControl(command_control));
}

Request Sentinel::Hsetnx(const std::string& key, const std::string& field,
                         const std::string& value,
                         const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"hsetnx", key, field, value}, key, true,
                     GetCommandControl(command_control));
}

Request Sentinel::Hstrlen(const std::string& key, const std::string& field,
                          const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"hstrlen", key, field}, key, false,
                     GetCommandControl(command_control));
}

Request Sentinel::Hvals(const std::string& key,
                        const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"hvals", key}, key, false,
                     GetCommandControl(command_control));
}

Request Sentinel::Incr(const std::string& key,
                       const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"incr", key}, key, true,
                     GetCommandControl(command_control));
}

Request Sentinel::Incrby(const std::string& key, int64_t incr,
                         const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"incrby", key, incr}, key, true,
                     GetCommandControl(command_control));
}

Request Sentinel::Incrbyfloat(const std::string& key, double incr,
                              const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"incrbyfloat", key, incr}, key, true,
                     GetCommandControl(command_control));
}

Request Sentinel::Keys(const std::string& keys_pattern, size_t shard,
                       const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"keys", keys_pattern}, shard, false,
                     GetCommandControl(command_control));
}

Request Sentinel::Lindex(const std::string& key, int64_t index,
                         const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"lindex", key, index}, key, false,
                     GetCommandControl(command_control));
}

Request Sentinel::Linsert(const std::string& key,
                          const std::string& before_after,
                          const std::string& pivot, const std::string& value,
                          const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"linsert", key, before_after, pivot, value}, key,
                     true, GetCommandControl(command_control));
}

Request Sentinel::Llen(const std::string& key,
                       const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"llen", key}, key, false,
                     GetCommandControl(command_control));
}

Request Sentinel::Lpop(const std::string& key,
                       const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"lpop", key}, key, true,
                     GetCommandControl(command_control));
}

Request Sentinel::Lpush(const std::string& key, const std::string& value,
                        const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"lpush", key, value}, key, true,
                     GetCommandControl(command_control));
}

Request Sentinel::Lpush(const std::string& key,
                        const std::vector<std::string>& values,
                        const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"lpush", key, values}, key, true,
                     GetCommandControl(command_control));
}

Request Sentinel::Lpushx(const std::string& key, const std::string& value,
                         const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"lpushx", key, value}, key, true,
                     GetCommandControl(command_control));
}

Request Sentinel::Lrange(const std::string& key, int64_t start, int64_t stop,
                         const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"lrange", key, start, stop}, key, false,
                     GetCommandControl(command_control));
}

Request Sentinel::Lrem(const std::string& key, int64_t count,
                       const std::string& value,
                       const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"lrem", key, count, value}, key, true,
                     GetCommandControl(command_control));
}

Request Sentinel::Lset(const std::string& key, int64_t index,
                       const std::string& value,
                       const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"lset", key, index, value}, key, true,
                     GetCommandControl(command_control));
}

Request Sentinel::Ltrim(const std::string& key, int64_t start, int64_t stop,
                        const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"ltrim", key, start, stop}, key, true,
                     GetCommandControl(command_control));
}

Request Sentinel::Mget(const std::vector<std::string>& keys,
                       const CommandControl& command_control) {
  UASSERT(!keys.empty());
  return MakeRequest(CmdArgs{"mget", keys}, keys[0], false,
                     GetCommandControl(command_control));
}

Request Sentinel::Persist(const std::string& key,
                          const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"persist", key}, key, true,
                     GetCommandControl(command_control));
}

Request Sentinel::Pexpire(const std::string& key, int64_t milliseconds,
                          const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"pexpire", key, milliseconds}, key, true,
                     GetCommandControl(command_control));
}

Request Sentinel::Pexpireat(const std::string& key,
                            int64_t milliseconds_timestamp,
                            const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"pexpireat", key, milliseconds_timestamp}, key,
                     true, GetCommandControl(command_control));
}

Request Sentinel::Pfadd(const std::string& key, const std::string& element,
                        const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"pfadd", key, element}, key, true,
                     GetCommandControl(command_control));
}

Request Sentinel::Pfadd(const std::string& key,
                        const std::vector<std::string>& elements,
                        const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"pfadd", key, elements}, key, true,
                     GetCommandControl(command_control));
}

Request Sentinel::Pfcount(const std::string& key,
                          const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"pfcount", key}, key, false,
                     GetCommandControl(command_control));
}

Request Sentinel::Pfcount(const std::vector<std::string>& keys,
                          const CommandControl& command_control) {
  UASSERT(!keys.empty());
  return MakeRequest(CmdArgs{"pfcount", keys}, keys[0], false,
                     GetCommandControl(command_control));
}

Request Sentinel::Pfmerge(const std::string& key,
                          const std::vector<std::string>& sourcekeys,
                          const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"pfmerge", key, sourcekeys}, key, true,
                     GetCommandControl(command_control));
}

Request Sentinel::Ping(const std::string& key,
                       const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"ping", key}, key, false,
                     GetCommandControl(command_control));
}

Request Sentinel::Psetex(const std::string& key, int64_t milliseconds,
                         const std::string& val,
                         const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"psetex", key, milliseconds, val}, key, true,
                     GetCommandControl(command_control));
}

Request Sentinel::Pttl(const std::string& key,
                       const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"pttl", key}, key, false,
                     GetCommandControl(command_control));
}

Request Sentinel::Restore(const std::string& key, int64_t ttl,
                          const std::string& serialized_value,
                          const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"restore", key, ttl, serialized_value}, key, true,
                     GetCommandControl(command_control));
}

Request Sentinel::Restore(const std::string& key, int64_t ttl,
                          const std::string& serialized_value,
                          const std::string& replace,
                          const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"restore", key, ttl, serialized_value, replace},
                     key, true, GetCommandControl(command_control));
}

Request Sentinel::RenameWithinShard(const std::string& key,
                                    const std::string& newkey,
                                    const CommandControl& command_control) {
  if (!command_control.force_shard_idx) CheckRenameParams(key, newkey);
  return MakeRequest(CmdArgs{"rename", key, newkey}, key, true,
                     GetCommandControl(command_control));
}

Request Sentinel::Rpop(const std::string& key,
                       const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"rpop", key}, key, true,
                     GetCommandControl(command_control));
}

Request Sentinel::Rpoplpush(const std::string& key,
                            const std::string& destination,
                            const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"rpoplpush", key, destination}, key, true,
                     GetCommandControl(command_control));
}

Request Sentinel::Rpush(const std::string& key, const std::string& value,
                        const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"rpush", key, value}, key, true,
                     GetCommandControl(command_control));
}

Request Sentinel::Rpush(const std::string& key, const std::string& value,
                        size_t shard, const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"rpush", key, value}, shard, true,
                     GetCommandControl(command_control));
}

Request Sentinel::Rpush(const std::string& key,
                        const std::vector<std::string>& values,
                        const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"rpush", key, values}, key, true,
                     GetCommandControl(command_control));
}

Request Sentinel::Rpushx(const std::string& key, const std::string& value,
                         const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"rpushx", key, value}, key, true,
                     GetCommandControl(command_control));
}

Request Sentinel::Sadd(const std::string& key, const std::string& member,
                       const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"sadd", key, member}, key, true,
                     GetCommandControl(command_control));
}

Request Sentinel::Sadd(const std::string& key,
                       const std::vector<std::string>& members,
                       const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"sadd", key, members}, key, true,
                     GetCommandControl(command_control));
}

Request Sentinel::Scan(size_t shard, const ScanOptions& options,
                       const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"scan", options}, shard, false,
                     GetCommandControl(command_control));
}

Request Sentinel::Scard(const std::string& key,
                        const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"scard", key}, key, false,
                     GetCommandControl(command_control));
}

Request Sentinel::Sdiff(const std::vector<std::string>& keys,
                        const CommandControl& command_control) {
  UASSERT(!keys.empty());
  return MakeRequest(CmdArgs{"sdiff", keys}, keys[0], false,
                     GetCommandControl(command_control));
}

Request Sentinel::Sdiffstore(const std::string& destination,
                             const std::vector<std::string>& keys,
                             const CommandControl& command_control) {
  UASSERT(!keys.empty());
  return MakeRequest(CmdArgs{"sdiffstore", destination, keys}, keys[0], true,
                     GetCommandControl(command_control));
}

Request Sentinel::Set(const std::string& key, const std::string& value,
                      const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"set", key, value}, key, true,
                     GetCommandControl(command_control));
}

Request Sentinel::Set(const std::string& key, std::string&& value,
                      const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"set", key, std::move(value)}, key, true,
                     GetCommandControl(command_control));
}

Request Sentinel::Set(const std::string& key, const std::string& value,
                      const SetOptions& options,
                      const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"set", key, value, options}, key, true,
                     GetCommandControl(command_control));
}

Request Sentinel::Setbit(const std::string& key, int64_t offset,
                         const std::string& value,
                         const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"setbit", key, offset, value}, key, true,
                     GetCommandControl(command_control));
}

Request Sentinel::Setex(const std::string& key, int64_t seconds,
                        const std::string& value,
                        const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"setex", key, seconds, value}, key, true,
                     GetCommandControl(command_control));
}

Request Sentinel::Setnx(const std::string& key, const std::string& value,
                        const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"setnx", key, value}, key, true,
                     GetCommandControl(command_control));
}

Request Sentinel::Setrange(const std::string& key, int64_t offset,
                           const std::string& value,
                           const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"setrange", key, offset, value}, key, true,
                     GetCommandControl(command_control));
}

Request Sentinel::Sinter(const std::vector<std::string>& keys,
                         const CommandControl& command_control) {
  UASSERT(!keys.empty());
  return MakeRequest(CmdArgs{"sinter", keys}, keys[0], false,
                     GetCommandControl(command_control));
}

Request Sentinel::Sinterstore(const std::string& destination,
                              const std::vector<std::string>& keys,
                              const CommandControl& command_control) {
  UASSERT(!keys.empty());
  return MakeRequest(CmdArgs{"sinterstore", destination, keys}, keys[0], true,
                     GetCommandControl(command_control));
}

Request Sentinel::Sismember(const std::string& key, const std::string& member,
                            const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"sismember", key, member}, key, false,
                     GetCommandControl(command_control));
}

Request Sentinel::Smembers(const std::string& key,
                           const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"smembers", key}, key, false,
                     GetCommandControl(command_control));
}

Request Sentinel::Smove(const std::string& key, const std::string& destination,
                        const std::string& member,
                        const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"smove", key, destination, member}, key, true,
                     GetCommandControl(command_control));
}

Request Sentinel::Spop(const std::string& key,
                       const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"spop", key}, key, true,
                     GetCommandControl(command_control));
}

Request Sentinel::Spop(const std::string& key, int64_t count,
                       const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"spop", key, count}, key, true,
                     GetCommandControl(command_control));
}

Request Sentinel::Srandmember(const std::string& key,
                              const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"srandmember", key}, key, false,
                     GetCommandControl(command_control));
}

Request Sentinel::Srandmember(const std::string& key, int64_t count,
                              const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"srandmember", key, count}, key, false,
                     GetCommandControl(command_control));
}

Request Sentinel::Srem(const std::string& key, const std::string& members,
                       const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"srem", key, members}, key, true,
                     GetCommandControl(command_control));
}

Request Sentinel::Srem(const std::string& key,
                       const std::vector<std::string>& members,
                       const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"srem", key, members}, key, true,
                     GetCommandControl(command_control));
}

Request Sentinel::strlen(const std::string& key,
                         const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"strlen", key}, key, false,
                     GetCommandControl(command_control));
}

Request Sentinel::Sunion(const std::vector<std::string>& keys,
                         const CommandControl& command_control) {
  UASSERT(!keys.empty());
  return MakeRequest(CmdArgs{"sunion", keys}, keys[0], false,
                     GetCommandControl(command_control));
}

Request Sentinel::Sunionstore(const std::string& destination,
                              const std::vector<std::string>& keys,
                              const CommandControl& command_control) {
  UASSERT(!keys.empty());
  return MakeRequest(CmdArgs{"sunionstore", destination, keys}, keys[0], true,
                     GetCommandControl(command_control));
}

Request Sentinel::Ttl(const std::string& key,
                      const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"ttl", key}, key, false,
                     GetCommandControl(command_control));
}

Request Sentinel::Type(const std::string& key,
                       const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"type", key}, key, false,
                     GetCommandControl(command_control));
}

Request Sentinel::Zadd(const std::string& key, double score,
                       const std::string& member,
                       const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"zadd", key, score, member}, key, true,
                     GetCommandControl(command_control));
}

Request Sentinel::Zadd(const std::string& key, double score,
                       const std::string& member, const ZaddOptions& options,
                       const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"zadd", key, options, score, member}, key, true,
                     GetCommandControl(command_control));
}

Request Sentinel::Zadd(
    const std::string& key,
    const std::vector<std::pair<double, std::string>>& scores_members,
    const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"zadd", key, scores_members}, key, true,
                     GetCommandControl(command_control));
}

Request Sentinel::Zadd(
    const std::string& key,
    const std::vector<std::pair<double, std::string>>& scores_members,
    const ZaddOptions& options, const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"zadd", key, options, scores_members}, key, true,
                     GetCommandControl(command_control));
}

Request Sentinel::Zcard(const std::string& key,
                        const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"zcard", key}, key, false,
                     GetCommandControl(command_control));
}

Request Sentinel::Zcount(const std::string& key, double min, double max,
                         const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"zcount", key, min, max}, key, false,
                     GetCommandControl(command_control));
}

Request Sentinel::Zcount(const std::string& key, const std::string& min,
                         const std::string& max,
                         const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"zcount", key, min, max}, key, false,
                     GetCommandControl(command_control));
}

Request Sentinel::Zincrby(const std::string& key, int64_t incr,
                          const std::string& member,
                          const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"zincrby", key, incr, member}, key, true,
                     GetCommandControl(command_control));
}

Request Sentinel::Zincrby(const std::string& key, double incr,
                          const std::string& member,
                          const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"zincrby", key, incr, member}, key, true,
                     GetCommandControl(command_control));
}

Request Sentinel::Zincrby(const std::string& key, const std::string& incr,
                          const std::string& member,
                          const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"zincrby", key, incr, member}, key, true,
                     GetCommandControl(command_control));
}

Request Sentinel::Zlexcount(const std::string& key, const std::string& min,
                            const std::string& max,
                            const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"zlexcount", key, min, max}, key, false,
                     GetCommandControl(command_control));
}

Request Sentinel::Zrange(const std::string& key, int64_t start, int64_t stop,
                         const ScoreOptions& score_options,
                         const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"zrange", key, start, stop, score_options}, key,
                     false, GetCommandControl(command_control));
}

Request Sentinel::Zrangebylex(const std::string& key, const std::string& min,
                              const std::string& max,
                              const RangeOptions& range_options,
                              const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"zrangebylex", key, min, max, range_options}, key,
                     false, GetCommandControl(command_control));
}

Request Sentinel::Zrangebyscore(const std::string& key, double min, double max,
                                const RangeScoreOptions& range_score_options,
                                const CommandControl& command_control) {
  return MakeRequest(
      CmdArgs{"zrangebyscore", key, min, max, range_score_options}, key, false,
      GetCommandControl(command_control));
}

Request Sentinel::Zrangebyscore(const std::string& key, const std::string& min,
                                const std::string& max,
                                const RangeScoreOptions& range_score_options,
                                const CommandControl& command_control) {
  return MakeRequest(
      CmdArgs{"zrangebyscore", key, min, max, range_score_options}, key, false,
      GetCommandControl(command_control));
}

Request Sentinel::Zrangebyscore(const std::string& key, const std::string& min,
                                const std::string& max,
                                const ScoreOptions& score_options,
                                const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"zrangebyscore", key, min, max, score_options},
                     key, false, GetCommandControl(command_control));
}

Request Sentinel::Zrank(const std::string& key, const std::string& member,
                        const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"zrank", key, member}, key, false,
                     GetCommandControl(command_control));
}

Request Sentinel::Zrem(const std::string& key, const std::string& member,
                       const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"zrem", key, member}, key, true,
                     GetCommandControl(command_control));
}

Request Sentinel::Zrem(const std::string& key,
                       const std::vector<std::string>& members,
                       const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"zrem", key, members}, key, true,
                     GetCommandControl(command_control));
}

Request Sentinel::Zremrangebylex(const std::string& key, const std::string& min,
                                 const std::string& max,
                                 const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"zremrangebylex", key, min, max}, key, true,
                     GetCommandControl(command_control));
}

Request Sentinel::Zremrangebyrank(const std::string& key, int64_t start,
                                  int64_t stop,
                                  const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"zremrangebyrank", key, start, stop}, key, true,
                     GetCommandControl(command_control));
}

Request Sentinel::Zremrangebyscore(const std::string& key, double min,
                                   double max,
                                   const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"zremrangebyscore", key, min, max}, key, true,
                     GetCommandControl(command_control));
}

Request Sentinel::Zremrangebyscore(const std::string& key,
                                   const std::string& min,
                                   const std::string& max,
                                   const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"zremrangebyscore", key, min, max}, key, true,
                     GetCommandControl(command_control));
}

Request Sentinel::Zrevrange(const std::string& key, int64_t start, int64_t stop,
                            const ScoreOptions& score_options,
                            const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"zrevrange", key, start, stop, score_options}, key,
                     false, GetCommandControl(command_control));
}

Request Sentinel::Zrevrangebylex(const std::string& key, const std::string& min,
                                 const std::string& max,
                                 const RangeOptions& range_options,
                                 const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"zrevrangebylex", key, min, max, range_options},
                     key, false, GetCommandControl(command_control));
}

Request Sentinel::Zrevrangebyscore(const std::string& key, double min,
                                   double max,
                                   const RangeScoreOptions& range_score_options,
                                   const CommandControl& command_control) {
  return MakeRequest(
      CmdArgs{"zrevrangebyscore", key, min, max, range_score_options}, key,
      false, GetCommandControl(command_control));
}

Request Sentinel::Zrevrangebyscore(const std::string& key,
                                   const std::string& min,
                                   const std::string& max,
                                   const RangeScoreOptions& range_score_options,
                                   const CommandControl& command_control) {
  return MakeRequest(
      CmdArgs{"zrevrangebyscore", key, min, max, range_score_options}, key,
      false, GetCommandControl(command_control));
}

Request Sentinel::Zrevrank(const std::string& key, const std::string& member,
                           const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"zrevrank", key, member}, key, false,
                     GetCommandControl(command_control));
}

Request Sentinel::Zscore(const std::string& key, const std::string& member,
                         const CommandControl& command_control) {
  return MakeRequest(CmdArgs{"zscore", key, member}, key, false,
                     GetCommandControl(command_control));
}

Request Sentinel::Publish(const std::string& channel, std::string message,
                          const CommandControl& command_control,
                          PubShard policy) {
  const auto shard = GetPublishShard(policy);
  return MakeRequest(CmdArgs{"PUBLISH", channel, std::move(message)}, shard,
                     true, GetCommandControl(command_control));
}

void Sentinel::OnSubscribeReply(const MessageCallback message_callback,
                                const SubscribeCallback subscribe_callback,
                                const UnsubscribeCallback unsubscribe_callback,
                                ReplyPtr reply) {
  if (!reply->data.IsArray()) return;
  const auto& reply_array = reply->data.GetArray();
  if (reply_array.size() != 3 || !reply_array[0].IsString()) return;
  if (!strcasecmp(reply_array[0].GetString().c_str(), "SUBSCRIBE")) {
    if (subscribe_callback)
      subscribe_callback(reply->server_id, reply_array[1].GetString(),
                         reply_array[2].GetInt());
  } else if (!strcasecmp(reply_array[0].GetString().c_str(), "UNSUBSCRIBE")) {
    if (unsubscribe_callback)
      unsubscribe_callback(reply->server_id, reply_array[1].GetString(),
                           reply_array[2].GetInt());
  } else if (!strcasecmp(reply_array[0].GetString().c_str(), "MESSAGE")) {
    if (message_callback)
      message_callback(reply->server_id, reply_array[1].GetString(),
                       reply_array[2].GetString());
  }
}

void Sentinel::OnPsubscribeReply(const PmessageCallback pmessage_callback,
                                 const SubscribeCallback subscribe_callback,
                                 const UnsubscribeCallback unsubscribe_callback,
                                 ReplyPtr reply) {
  if (!reply->data.IsArray()) return;
  const auto& reply_array = reply->data.GetArray();
  if (!reply_array[0].IsString()) return;
  if (!strcasecmp(reply_array[0].GetString().c_str(), "PSUBSCRIBE")) {
    if (reply_array.size() == 3 && subscribe_callback)
      subscribe_callback(reply->server_id, reply_array[1].GetString(),
                         reply_array[2].GetInt());
  } else if (!strcasecmp(reply_array[0].GetString().c_str(), "PUNSUBSCRIBE")) {
    if (reply_array.size() == 3 && unsubscribe_callback)
      unsubscribe_callback(reply->server_id, reply_array[1].GetString(),
                           reply_array[2].GetInt());
  } else if (!strcasecmp(reply_array[0].GetString().c_str(), "PMESSAGE")) {
    if (reply_array.size() == 4 && pmessage_callback)
      pmessage_callback(reply->server_id, reply_array[1].GetString(),
                        reply_array[2].GetString(), reply_array[3].GetString());
  }
}

CommandControl Sentinel::GetCommandControl(const CommandControl& cc) const {
  return secdist_default_command_control_
      .MergeWith(*config_default_command_control_.Get())
      .MergeWith(cc);
}

void Sentinel::SetConfigDefaultCommandControl(
    const std::shared_ptr<CommandControl>& cc) {
  config_default_command_control_.Set(cc);
}

size_t Sentinel::GetPublishShard(PubShard policy) {
  switch (policy) {
    case PubShard::kZeroShard:
      return 0;

    case PubShard::kRoundRobin:
      return ++publish_shard_ % impl_->ShardsCount();
  }

  return 0;
}

std::vector<std::shared_ptr<const Shard>> Sentinel::GetMasterShards() const {
  return impl_->GetMasterShards();
}

void Sentinel::CheckRenameParams(const std::string& key,
                                 const std::string& newkey) {
  if (ShardByKey(key) != ShardByKey(newkey))
    throw InvalidArgumentException(
        "key and newkey must have the same shard key");
}

}  // namespace redis
