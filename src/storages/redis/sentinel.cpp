#include "sentinel.hpp"

#include <algorithm>
#include <cstring>
#include <deque>
#include <limits>
#include <map>
#include <set>
#include <sstream>
#include <thread>
#include <unordered_map>

#include <hiredis/adapters/libev.h>
#include <hiredis/hiredis.h>

#include <engine/async_task.hpp>
#include <engine/ev/thread_control.hpp>
#include <logging/logger.hpp>

#include "redis.hpp"
#include "sentinel_impl.hpp"
#include "transaction.hpp"

namespace storages {
namespace redis {

Sentinel::Sentinel(
    const engine::ev::ThreadControl& sentinel_thread_control,
    engine::ev::ThreadPool& thread_pool, engine::TaskProcessor& task_processor,
    const std::vector<std::string>& shards,
    const std::vector<ConnectionInfo>& conns, const std::string& password,
    ReadyChangeCallback ready_callback, std::unique_ptr<KeyShard>&& key_shard,
    CommandControl command_control, bool track_masters, bool track_slaves)
    : engine::ev::ImplInEvLoop<SentinelImpl>(
          sentinel_thread_control, thread_pool, shards, conns, password,
          ready_callback, std::move(key_shard), track_masters, track_slaves),
      default_command_control_(command_control),
      task_processor_(task_processor) {}

Sentinel::~Sentinel() {}

std::shared_ptr<Sentinel> Sentinel::CreateSentinel(
    const secdist::RedisSettings& settings,
    const engine::ev::ThreadControl& sentinel_thread_control,
    engine::ev::ThreadPool& thread_pool,
    engine::TaskProcessor& task_processor) {
  auto ready_callback = [](size_t shard, const std::string& shard_name,
                           bool master, bool ready) {
    LOG_INFO() << "redis: ready_callback:"
               << " shard=" << shard << " shard_name=" << shard_name
               << " master=" << master << " ready=" << ready;
  };
  return CreateSentinel(settings, sentinel_thread_control, thread_pool,
                        task_processor, std::move(ready_callback));
}

std::shared_ptr<Sentinel> Sentinel::CreateSentinel(
    const secdist::RedisSettings& settings,
    const engine::ev::ThreadControl& sentinel_thread_control,
    engine::ev::ThreadPool& thread_pool, engine::TaskProcessor& task_processor,
    Sentinel::ReadyChangeCallback&& ready_callback) {
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
    conns.push_back(redis::ConnectionInfo{sentinel.host, sentinel.port, ""});
  }
  const auto& cc_source = settings.command_control;
  CommandControl command_control = command_control_init;
  if (cc_source.timeout_single > std::chrono::milliseconds(0))
    command_control.timeout_single = cc_source.timeout_single;
  if (cc_source.timeout_all > std::chrono::milliseconds(0))
    command_control.timeout_all = cc_source.timeout_all;
  if (cc_source.max_retries > 0)
    command_control.max_retries = cc_source.max_retries;
  LOG_DEBUG() << "redis command_control:"
              << "  timeout_single = "
              << std::chrono::duration_cast<std::chrono::milliseconds>(
                     command_control.timeout_single)
                     .count()
              << "ms"
              << "  timeout_all = "
              << std::chrono::duration_cast<std::chrono::milliseconds>(
                     command_control.timeout_all)
                     .count()
              << "ms"
              << "  max_retries = " << command_control.max_retries;
  std::shared_ptr<redis::Sentinel> client;
  if (!shards.empty() && !conns.empty())
    client = std::make_shared<redis::Sentinel>(
        sentinel_thread_control, thread_pool, task_processor, shards, conns,
        password, std::move(ready_callback),
        std::make_unique<redis::KeyShardCrc32>(shards.size()), command_control,
        true, true);

  return client;
}

void Sentinel::AsyncCommand(CommandPtr command, bool master, size_t shard) {
  try {
    impl_->AsyncCommand({/*.command = */ command,
                         /*.master = */ master,
                         /*.shard = */ shard,
                         /*.start = */ std::chrono::steady_clock::now()});
  } catch (const std::exception& ex) {
    LOG_ERROR() << "exception in Sentinel::async_command '" << ex.what() << "'";
  }
}

void Sentinel::AsyncCommand(CommandPtr command, const std::string& key,
                            bool master) {
  try {
    impl_->AsyncCommand({/*.command = */ command,
                         /*.master = */ master,
                         /*.shard = */ impl_->ShardByKey(key),
                         /*.start = */ std::chrono::steady_clock::now()});
  } catch (const std::exception& ex) {
    LOG_ERROR() << "exception in Sentinel::async_command '" << ex.what() << "'";
  }
}

size_t Sentinel::ShardByKey(const std::string& key) const {
  return impl_->ShardByKey(key);
}

size_t Sentinel::ShardCount() const { return impl_->ShardCount(); }

Stat Sentinel::GetStat() { return impl_->GetStat(); }

Request Sentinel::MakeRequest(
    CmdArgs&& args, const std::string& key, bool master /* = true*/,
    const CommandControl& command_control /* = CommandControl()*/,
    bool skip_status /* = false*/) {
  return Request(*this, std::move(args), key, master, command_control,
                 skip_status);
}

Request Sentinel::MakeRequest(
    CmdArgs&& args, size_t shard, bool master /* = true*/,
    const CommandControl& command_control /* = CommandControl()*/,
    bool skip_status /* = false*/) {
  return Request(*this, std::move(args), shard, master, command_control,
                 skip_status);
}

Request Sentinel::Append(const std::string& key, const std::string& value,
                         const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"append", key, value}, key, true,
                      GetCommandControl(command_control)));
}

void Sentinel::Append(const std::string& key, const std::string& value,
                      Callback&& callback,
                      const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"append", key, value}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, true);
}

Request Sentinel::Bitcount(const std::string& key,
                           const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"bitcount", key}, key, false,
                      GetCommandControl(command_control)));
}

void Sentinel::Bitcount(const std::string& key, Callback&& callback,
                        const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"bitcount", key}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, false);
}

Request Sentinel::Bitcount(const std::string& key, int64_t start, int64_t end,
                           const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"bitcount", key, start, end}, key, false,
                      GetCommandControl(command_control)));
}

void Sentinel::Bitcount(const std::string& key, int64_t start, int64_t end,
                        Callback&& callback,
                        const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"bitcount", key, start, end}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, false);
}

Request Sentinel::Bitop(const std::string& operation,
                        const std::string& destkey, const std::string& key,
                        const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"bitop", operation, destkey, key}, key, true,
                      GetCommandControl(command_control)));
}

void Sentinel::Bitop(const std::string& operation, const std::string& destkey,
                     const std::string& key, Callback&& callback,
                     const CommandControl& command_control) {
  AsyncCommand(PrepareCommand(CmdArgs{"bitop", operation, destkey, key},
                              std::move(callback), task_processor_,
                              GetCommandControl(command_control)),
               key, true);
}

Request Sentinel::Bitop(const std::string& operation,
                        const std::string& destkey,
                        const std::vector<std::string>& keys,
                        const CommandControl& command_control) {
  assert(!keys.empty());
  return (MakeRequest(CmdArgs{"bitop", operation, destkey, keys}, keys[0], true,
                      GetCommandControl(command_control)));
}

void Sentinel::Bitop(const std::string& operation, const std::string& destkey,
                     const std::vector<std::string>& keys, Callback&& callback,
                     const CommandControl& command_control) {
  assert(!keys.empty());
  AsyncCommand(PrepareCommand(CmdArgs{"bitop", operation, destkey, keys},
                              std::move(callback), task_processor_,
                              GetCommandControl(command_control)),
               keys[0], true);
}

Request Sentinel::Bitpos(const std::string& key, int bit,
                         const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"bitpos", key, bit}, key, false,
                      GetCommandControl(command_control)));
}

void Sentinel::Bitpos(const std::string& key, int bit, Callback&& callback,
                      const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"bitpos", key, bit}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, false);
}

Request Sentinel::Bitpos(const std::string& key, int bit, int64_t start,
                         const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"bitpos", key, bit, start}, key, false,
                      GetCommandControl(command_control)));
}

void Sentinel::Bitpos(const std::string& key, int bit, int64_t start,
                      Callback&& callback,
                      const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"bitpos", key, bit, start}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, false);
}

Request Sentinel::Bitpos(const std::string& key, int bit, int64_t start,
                         int64_t end, const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"bitpos", key, bit, start, end}, key, false,
                      GetCommandControl(command_control)));
}

void Sentinel::Bitpos(const std::string& key, int bit, int64_t start,
                      int64_t end, Callback&& callback,
                      const CommandControl& command_control) {
  AsyncCommand(PrepareCommand(CmdArgs{"bitpos", key, bit, start, end},
                              std::move(callback), task_processor_,
                              GetCommandControl(command_control)),
               key, false);
}

Request Sentinel::Blpop(const std::string& key, int64_t timeout,
                        const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"blpop", key, timeout}, key, true,
                      GetCommandControl(command_control)));
}

void Sentinel::Blpop(const std::string& key, int64_t timeout,
                     Callback&& callback,
                     const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"blpop", key, timeout}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, true);
}

Request Sentinel::Blpop(const std::vector<std::string>& keys, int64_t timeout,
                        const CommandControl& command_control) {
  assert(!keys.empty());
  return (MakeRequest(CmdArgs{"blpop", keys, timeout}, keys[0], true,
                      GetCommandControl(command_control)));
}

void Sentinel::Blpop(const std::vector<std::string>& keys, int64_t timeout,
                     Callback&& callback,
                     const CommandControl& command_control) {
  assert(!keys.empty());
  AsyncCommand(
      PrepareCommand(CmdArgs{"blpop", keys, timeout}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      keys[0], true);
}

Request Sentinel::Brpop(const std::string& key, int64_t timeout,
                        const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"brpop", key, timeout}, key, true,
                      GetCommandControl(command_control)));
}

void Sentinel::Brpop(const std::string& key, int64_t timeout,
                     Callback&& callback,
                     const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"brpop", key, timeout}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, true);
}

Request Sentinel::Brpop(const std::vector<std::string>& keys, int64_t timeout,
                        const CommandControl& command_control) {
  assert(!keys.empty());
  return (MakeRequest(CmdArgs{"brpop", keys, timeout}, keys[0], true,
                      GetCommandControl(command_control)));
}

void Sentinel::Brpop(const std::vector<std::string>& keys, int64_t timeout,
                     Callback&& callback,
                     const CommandControl& command_control) {
  assert(!keys.empty());
  AsyncCommand(
      PrepareCommand(CmdArgs{"brpop", keys, timeout}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      keys[0], true);
}

Request Sentinel::ClusterKeyslot(const std::string& key,
                                 const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"cluster_keyslot", key}, key, false,
                      GetCommandControl(command_control)));
}

void Sentinel::ClusterKeyslot(const std::string& key, Callback&& callback,
                              const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"cluster_keyslot", key}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, false);
}

Request Sentinel::Decr(const std::string& key,
                       const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"decr", key}, key, true,
                      GetCommandControl(command_control)));
}

void Sentinel::Decr(const std::string& key, Callback&& callback,
                    const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"decr", key}, std::move(callback), task_processor_,
                     GetCommandControl(command_control)),
      key, true);
}

Request Sentinel::Decrby(const std::string& key, int64_t val,
                         const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"decrby", key, val}, key, true,
                      GetCommandControl(command_control)));
}

void Sentinel::Decrby(const std::string& key, int64_t val, Callback&& callback,
                      const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"decrby", key, val}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, true);
}

Request Sentinel::Del(const std::string& key,
                      const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"del", key}, key, true,
                      GetCommandControl(command_control)));
}

void Sentinel::Del(const std::string& key, Callback&& callback,
                   const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"del", key}, std::move(callback), task_processor_,
                     GetCommandControl(command_control)),
      key, true);
}

Request Sentinel::Del(const std::vector<std::string>& keys,
                      const CommandControl& command_control) {
  assert(!keys.empty());
  return (MakeRequest(CmdArgs{"del", keys}, keys[0], true,
                      GetCommandControl(command_control)));
}

void Sentinel::Del(const std::vector<std::string>& keys, Callback&& callback,
                   const CommandControl& command_control) {
  assert(!keys.empty());
  AsyncCommand(
      PrepareCommand(CmdArgs{"del", keys}, std::move(callback), task_processor_,
                     GetCommandControl(command_control)),
      keys[0], true);
}

Request Sentinel::Dump(const std::string& key,
                       const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"dump", key}, key, false,
                      GetCommandControl(command_control)));
}

void Sentinel::Dump(const std::string& key, Callback&& callback,
                    const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"dump", key}, std::move(callback), task_processor_,
                     GetCommandControl(command_control)),
      key, false);
}

Request Sentinel::Eval(const std::string& script, int64_t numkeys,
                       const std::vector<std::string>& keys,
                       const std::vector<std::string>& args,
                       const CommandControl& command_control) {
  assert(!keys.empty());
  return (MakeRequest(CmdArgs{"eval", script, numkeys, keys, args}, keys[0],
                      true, GetCommandControl(command_control)));
}

void Sentinel::Eval(const std::string& script, int64_t numkeys,
                    const std::vector<std::string>& keys,
                    const std::vector<std::string>& args, Callback&& callback,
                    const CommandControl& command_control) {
  assert(!keys.empty());
  AsyncCommand(PrepareCommand(CmdArgs{"eval", script, numkeys, keys, args},
                              std::move(callback), task_processor_,
                              GetCommandControl(command_control)),
               keys[0], true);
}

Request Sentinel::Evalsha(const std::string& sha1, int64_t numkeys,
                          const std::vector<std::string>& keys,
                          const std::vector<std::string>& args,
                          const CommandControl& command_control) {
  assert(!keys.empty());
  return (MakeRequest(CmdArgs{"evalsha", sha1, numkeys, keys, args}, keys[0],
                      true, GetCommandControl(command_control)));
}

void Sentinel::Evalsha(const std::string& sha1, int64_t numkeys,
                       const std::vector<std::string>& keys,
                       const std::vector<std::string>& args,
                       Callback&& callback,
                       const CommandControl& command_control) {
  assert(!keys.empty());
  AsyncCommand(PrepareCommand(CmdArgs{"evalsha", sha1, numkeys, keys, args},
                              std::move(callback), task_processor_,
                              GetCommandControl(command_control)),
               keys[0], true);
}

Request Sentinel::Exists(const std::string& key,
                         const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"exists", key}, key, false,
                      GetCommandControl(command_control)));
}

void Sentinel::Exists(const std::string& key, Callback&& callback,
                      const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"exists", key}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, false);
}

Request Sentinel::Exists(const std::vector<std::string>& keys,
                         const CommandControl& command_control) {
  assert(!keys.empty());
  return (MakeRequest(CmdArgs{"exists", keys}, keys[0], false,
                      GetCommandControl(command_control)));
}

void Sentinel::Exists(const std::vector<std::string>& keys, Callback&& callback,
                      const CommandControl& command_control) {
  assert(!keys.empty());
  AsyncCommand(
      PrepareCommand(CmdArgs{"exists", keys}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      keys[0], false);
}

Request Sentinel::Expire(const std::string& key, std::chrono::seconds seconds,
                         const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"expire", key, seconds.count()}, key, true,
                      GetCommandControl(command_control)));
}

void Sentinel::Expire(const std::string& key, std::chrono::seconds seconds,
                      Callback&& callback,
                      const CommandControl& command_control) {
  AsyncCommand(PrepareCommand(CmdArgs{"expire", key, seconds.count()},
                              std::move(callback), task_processor_,
                              GetCommandControl(command_control)),
               key, true);
}

Request Sentinel::Expireat(const std::string& key, int64_t timestamp,
                           const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"expireat", key, timestamp}, key, true,
                      GetCommandControl(command_control)));
}

void Sentinel::Expireat(const std::string& key, int64_t timestamp,
                        Callback&& callback,
                        const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"expireat", key, timestamp}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, true);
}

Request Sentinel::Geoadd(const std::string& key,
                         const GeoaddArg& lon_lat_member,
                         const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"geoadd", key, lon_lat_member}, key, true,
                      GetCommandControl(command_control)));
}

void Sentinel::Geoadd(const std::string& key, const GeoaddArg& lon_lat_member,
                      Callback&& callback,
                      const CommandControl& command_control) {
  AsyncCommand(PrepareCommand(CmdArgs{"geoadd", key, lon_lat_member},
                              std::move(callback), task_processor_,
                              GetCommandControl(command_control)),
               key, true);
}

Request Sentinel::Geoadd(const std::string& key,
                         const std::vector<GeoaddArg>& lon_lat_member,
                         const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"geoadd", key, lon_lat_member}, key, true,
                      GetCommandControl(command_control)));
}

void Sentinel::Geoadd(const std::string& key,
                      const std::vector<GeoaddArg>& lon_lat_member,
                      Callback&& callback,
                      const CommandControl& command_control) {
  AsyncCommand(PrepareCommand(CmdArgs{"geoadd", key, lon_lat_member},
                              std::move(callback), task_processor_,
                              GetCommandControl(command_control)),
               key, true);
}

Request Sentinel::Geodist(const std::string& key, const std::string& member_1,
                          const std::string& member_2,
                          const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"geodist", key, member_1, member_2}, key, false,
                      GetCommandControl(command_control)));
}

void Sentinel::Geodist(const std::string& key, const std::string& member_1,
                       const std::string& member_2, Callback&& callback,
                       const CommandControl& command_control) {
  AsyncCommand(PrepareCommand(CmdArgs{"geodist", key, member_1, member_2},
                              std::move(callback), task_processor_,
                              GetCommandControl(command_control)),
               key, false);
}

Request Sentinel::Geodist(const std::string& key, const std::string& member_1,
                          const std::string& member_2, const std::string& unit,
                          const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"geodist", key, member_1, member_2, unit}, key,
                      false, GetCommandControl(command_control)));
}

void Sentinel::Geodist(const std::string& key, const std::string& member_1,
                       const std::string& member_2, const std::string& unit,
                       Callback&& callback,
                       const CommandControl& command_control) {
  AsyncCommand(PrepareCommand(CmdArgs{"geodist", key, member_1, member_2, unit},
                              std::move(callback), task_processor_,
                              GetCommandControl(command_control)),
               key, false);
}

Request Sentinel::Geohash(const std::string& key, const std::string& member,
                          const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"geohash", key, member}, key, false,
                      GetCommandControl(command_control)));
}

void Sentinel::Geohash(const std::string& key, const std::string& member,
                       Callback&& callback,
                       const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"geohash", key, member}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, false);
}

Request Sentinel::Geohash(const std::string& key,
                          const std::vector<std::string>& members,
                          const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"geohash", key, members}, key, false,
                      GetCommandControl(command_control)));
}

void Sentinel::Geohash(const std::string& key,
                       const std::vector<std::string>& members,
                       Callback&& callback,
                       const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"geohash", key, members}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, false);
}

Request Sentinel::Geopos(const std::string& key, const std::string& member,
                         const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"geopos", key, member}, key, false,
                      GetCommandControl(command_control)));
}

void Sentinel::Geopos(const std::string& key, const std::string& member,
                      Callback&& callback,
                      const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"geopos", key, member}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, false);
}

Request Sentinel::Geopos(const std::string& key,
                         const std::vector<std::string>& members,
                         const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"geopos", key, members}, key, false,
                      GetCommandControl(command_control)));
}

void Sentinel::Geopos(const std::string& key,
                      const std::vector<std::string>& members,
                      Callback&& callback,
                      const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"geopos", key, members}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, false);
}

Request Sentinel::Georadius(const std::string& key, double lon, double lat,
                            double radius, const GeoradiusOptions& options,
                            const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"georadius", key, lon, lat, radius, options}, key,
                      false, GetCommandControl(command_control)));
}

void Sentinel::Georadius(const std::string& key, double lon, double lat,
                         double radius, const GeoradiusOptions& options,
                         Callback&& callback,
                         const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"georadius", key, lon, lat, radius, options},
                     std::move(callback), task_processor_,
                     GetCommandControl(command_control)),
      key, false);
}

Request Sentinel::Georadius(const std::string& key, double lon, double lat,
                            double radius, const std::string& unit,
                            const GeoradiusOptions& options,
                            const CommandControl& command_control) {
  return (
      MakeRequest(CmdArgs{"georadius", key, lon, lat, radius, unit, options},
                  key, false, GetCommandControl(command_control)));
}

void Sentinel::Georadius(const std::string& key, double lon, double lat,
                         double radius, const std::string& unit,
                         const GeoradiusOptions& options, Callback&& callback,
                         const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"georadius", key, lon, lat, radius, unit, options},
                     std::move(callback), task_processor_,
                     GetCommandControl(command_control)),
      key, false);
}

Request Sentinel::Georadiusbymember(const std::string& key,
                                    const std::string& member, double radius,
                                    const GeoradiusOptions& options,
                                    const CommandControl& command_control) {
  return (
      MakeRequest(CmdArgs{"georadiusbymember", key, member, radius, options},
                  key, false, GetCommandControl(command_control)));
}

void Sentinel::Georadiusbymember(const std::string& key,
                                 const std::string& member, double radius,
                                 const GeoradiusOptions& options,
                                 Callback&& callback,
                                 const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"georadiusbymember", key, member, radius, options},
                     std::move(callback), task_processor_,
                     GetCommandControl(command_control)),
      key, false);
}

Request Sentinel::Georadiusbymember(const std::string& key,
                                    const std::string& member, double radius,
                                    const std::string& unit,
                                    const GeoradiusOptions& options,
                                    const CommandControl& command_control) {
  return (MakeRequest(
      CmdArgs{"georadiusbymember", key, member, radius, unit, options}, key,
      false, GetCommandControl(command_control)));
}

void Sentinel::Georadiusbymember(const std::string& key,
                                 const std::string& member, double radius,
                                 const std::string& unit,
                                 const GeoradiusOptions& options,
                                 Callback&& callback,
                                 const CommandControl& command_control) {
  AsyncCommand(PrepareCommand(CmdArgs{"georadiusbymember", key, member, radius,
                                      unit, options},
                              std::move(callback), task_processor_,
                              GetCommandControl(command_control)),
               key, false);
}

Request Sentinel::Get(const std::string& key,
                      const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"get", key}, key, false,
                      GetCommandControl(command_control)));
}

void Sentinel::Get(const std::string& key, Callback&& callback,
                   const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"get", key}, std::move(callback), task_processor_,
                     GetCommandControl(command_control)),
      key, false);
}

Request Sentinel::Getbit(const std::string& key, int64_t offset,
                         const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"getbit", key, offset}, key, false,
                      GetCommandControl(command_control)));
}

void Sentinel::Getbit(const std::string& key, int64_t offset,
                      Callback&& callback,
                      const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"getbit", key, offset}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, false);
}

Request Sentinel::Getrange(const std::string& key, int64_t start, int64_t end,
                           const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"getrange", key, start, end}, key, false,
                      GetCommandControl(command_control)));
}

void Sentinel::Getrange(const std::string& key, int64_t start, int64_t end,
                        Callback&& callback,
                        const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"getrange", key, start, end}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, false);
}

Request Sentinel::Getset(const std::string& key, const std::string& val,
                         const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"getset", key, val}, key, true,
                      GetCommandControl(command_control)));
}

void Sentinel::Getset(const std::string& key, const std::string& val,
                      Callback&& callback,
                      const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"getset", key, val}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, true);
}

Request Sentinel::Hdel(const std::string& key, const std::string& field,
                       const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"hdel", key, field}, key, true,
                      GetCommandControl(command_control)));
}

void Sentinel::Hdel(const std::string& key, const std::string& field,
                    Callback&& callback,
                    const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"hdel", key, field}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, true);
}

Request Sentinel::Hdel(const std::string& key,
                       const std::vector<std::string>& fields,
                       const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"hdel", key, fields}, key, true,
                      GetCommandControl(command_control)));
}

void Sentinel::Hdel(const std::string& key,
                    const std::vector<std::string>& fields, Callback&& callback,
                    const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"hdel", key, fields}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, true);
}

Request Sentinel::Hexists(const std::string& key, const std::string& field,
                          const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"hexists", key, field}, key, false,
                      GetCommandControl(command_control)));
}

void Sentinel::Hexists(const std::string& key, const std::string& field,
                       Callback&& callback,
                       const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"hexists", key, field}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, false);
}

Request Sentinel::Hget(const std::string& key, const std::string& field,
                       const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"hget", key, field}, key, false,
                      GetCommandControl(command_control)));
}

void Sentinel::Hget(const std::string& key, const std::string& field,
                    Callback&& callback,
                    const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"hget", key, field}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, false);
}

Request Sentinel::Hgetall(const std::string& key,
                          const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"hgetall", key}, key, false,
                      GetCommandControl(command_control)));
}

void Sentinel::Hgetall(const std::string& key, Callback&& callback,
                       const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"hgetall", key}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, false);
}

Request Sentinel::Hincrby(const std::string& key, const std::string& field,
                          int64_t incr, const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"hincrby", key, field, incr}, key, true,
                      GetCommandControl(command_control)));
}

void Sentinel::Hincrby(const std::string& key, const std::string& field,
                       int64_t incr, Callback&& callback,
                       const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"hincrby", key, field, incr}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, true);
}

Request Sentinel::Hincrbyfloat(const std::string& key, const std::string& field,
                               double incr,
                               const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"hincrbyfloat", key, field, incr}, key, true,
                      GetCommandControl(command_control)));
}

void Sentinel::Hincrbyfloat(const std::string& key, const std::string& field,
                            double incr, Callback&& callback,
                            const CommandControl& command_control) {
  AsyncCommand(PrepareCommand(CmdArgs{"hincrbyfloat", key, field, incr},
                              std::move(callback), task_processor_,
                              GetCommandControl(command_control)),
               key, true);
}

Request Sentinel::Hkeys(const std::string& key,
                        const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"hkeys", key}, key, false,
                      GetCommandControl(command_control)));
}

void Sentinel::Hkeys(const std::string& key, Callback&& callback,
                     const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"hkeys", key}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, false);
}

Request Sentinel::Hlen(const std::string& key,
                       const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"hlen", key}, key, false,
                      GetCommandControl(command_control)));
}

void Sentinel::Hlen(const std::string& key, Callback&& callback,
                    const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"hlen", key}, std::move(callback), task_processor_,
                     GetCommandControl(command_control)),
      key, false);
}

Request Sentinel::Hmget(const std::string& key,
                        const std::vector<std::string>& fields,
                        const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"hmget", key, fields}, key, false,
                      GetCommandControl(command_control)));
}

void Sentinel::Hmget(const std::string& key,
                     const std::vector<std::string>& fields,
                     Callback&& callback,
                     const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"hmget", key, fields}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, false);
}

Request Sentinel::Hmset(
    const std::string& key,
    const std::vector<std::pair<std::string, std::string>>& field_val,
    const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"hmset", key, field_val}, key, true,
                      GetCommandControl(command_control)));
}

void Sentinel::Hmset(
    const std::string& key,
    const std::vector<std::pair<std::string, std::string>>& field_val,
    Callback&& callback, const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"hmset", key, field_val}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, true);
}

Request Sentinel::Hset(const std::string& key, const std::string& field,
                       const std::string& value,
                       const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"hset", key, field, value}, key, true,
                      GetCommandControl(command_control)));
}

void Sentinel::Hset(const std::string& key, const std::string& field,
                    const std::string& value, Callback&& callback,
                    const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"hset", key, field, value}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, true);
}

Request Sentinel::Hsetnx(const std::string& key, const std::string& field,
                         const std::string& value,
                         const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"hsetnx", key, field, value}, key, true,
                      GetCommandControl(command_control)));
}

void Sentinel::Hsetnx(const std::string& key, const std::string& field,
                      const std::string& value, Callback&& callback,
                      const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"hsetnx", key, field, value}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, true);
}

Request Sentinel::Hstrlen(const std::string& key, const std::string& field,
                          const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"hstrlen", key, field}, key, false,
                      GetCommandControl(command_control)));
}

void Sentinel::Hstrlen(const std::string& key, const std::string& field,
                       Callback&& callback,
                       const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"hstrlen", key, field}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, false);
}

Request Sentinel::Hvals(const std::string& key,
                        const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"hvals", key}, key, false,
                      GetCommandControl(command_control)));
}

void Sentinel::Hvals(const std::string& key, Callback&& callback,
                     const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"hvals", key}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, false);
}

Request Sentinel::Incr(const std::string& key,
                       const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"incr", key}, key, true,
                      GetCommandControl(command_control)));
}

void Sentinel::Incr(const std::string& key, Callback&& callback,
                    const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"incr", key}, std::move(callback), task_processor_,
                     GetCommandControl(command_control)),
      key, true);
}

Request Sentinel::Incrby(const std::string& key, int64_t incr,
                         const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"incrby", key, incr}, key, true,
                      GetCommandControl(command_control)));
}

void Sentinel::Incrby(const std::string& key, int64_t incr, Callback&& callback,
                      const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"incrby", key, incr}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, true);
}

Request Sentinel::Incrbyfloat(const std::string& key, double incr,
                              const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"incrbyfloat", key, incr}, key, true,
                      GetCommandControl(command_control)));
}

void Sentinel::Incrbyfloat(const std::string& key, double incr,
                           Callback&& callback,
                           const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"incrbyfloat", key, incr}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, true);
}

Request Sentinel::Keys(const std::string& keys_pattern, size_t shard,
                       const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"keys", keys_pattern}, shard, false,
                      GetCommandControl(command_control)));
}

void Sentinel::Keys(const std::string& keys_pattern, size_t shard,
                    Callback&& callback,
                    const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"keys", keys_pattern}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      false, shard);
}

Request Sentinel::Lindex(const std::string& key, int64_t index,
                         const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"lindex", key, index}, key, false,
                      GetCommandControl(command_control)));
}

void Sentinel::Lindex(const std::string& key, int64_t index,
                      Callback&& callback,
                      const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"lindex", key, index}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, false);
}

Request Sentinel::Linsert(const std::string& key,
                          const std::string& before_after,
                          const std::string& pivot, const std::string& value,
                          const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"linsert", key, before_after, pivot, value}, key,
                      true, GetCommandControl(command_control)));
}

void Sentinel::Linsert(const std::string& key, const std::string& before_after,
                       const std::string& pivot, const std::string& value,
                       Callback&& callback,
                       const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"linsert", key, before_after, pivot, value},
                     std::move(callback), task_processor_,
                     GetCommandControl(command_control)),
      key, true);
}

Request Sentinel::Llen(const std::string& key,
                       const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"llen", key}, key, false,
                      GetCommandControl(command_control)));
}

void Sentinel::Llen(const std::string& key, Callback&& callback,
                    const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"llen", key}, std::move(callback), task_processor_,
                     GetCommandControl(command_control)),
      key, false);
}

Request Sentinel::Lpop(const std::string& key,
                       const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"lpop", key}, key, true,
                      GetCommandControl(command_control)));
}

void Sentinel::Lpop(const std::string& key, Callback&& callback,
                    const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"lpop", key}, std::move(callback), task_processor_,
                     GetCommandControl(command_control)),
      key, true);
}

Request Sentinel::Lpush(const std::string& key, const std::string& value,
                        const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"lpush", key, value}, key, true,
                      GetCommandControl(command_control)));
}

void Sentinel::Lpush(const std::string& key, const std::string& value,
                     Callback&& callback,
                     const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"lpush", key, value}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, true);
}

Request Sentinel::Lpush(const std::string& key,
                        const std::vector<std::string>& values,
                        const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"lpush", key, values}, key, true,
                      GetCommandControl(command_control)));
}

void Sentinel::Lpush(const std::string& key,
                     const std::vector<std::string>& values,
                     Callback&& callback,
                     const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"lpush", key, values}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, true);
}

Request Sentinel::Lpushx(const std::string& key, const std::string& value,
                         const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"lpushx", key, value}, key, true,
                      GetCommandControl(command_control)));
}

void Sentinel::Lpushx(const std::string& key, const std::string& value,
                      Callback&& callback,
                      const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"lpushx", key, value}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, true);
}

Request Sentinel::Lrange(const std::string& key, int64_t start, int64_t stop,
                         const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"lrange", key, start, stop}, key, false,
                      GetCommandControl(command_control)));
}

void Sentinel::Lrange(const std::string& key, int64_t start, int64_t stop,
                      Callback&& callback,
                      const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"lrange", key, start, stop}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, false);
}

Request Sentinel::Lrem(const std::string& key, int64_t count,
                       const std::string& value,
                       const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"lrem", key, count, value}, key, true,
                      GetCommandControl(command_control)));
}

void Sentinel::Lrem(const std::string& key, int64_t count,
                    const std::string& value, Callback&& callback,
                    const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"lrem", key, count, value}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, true);
}

Request Sentinel::Lset(const std::string& key, int64_t index,
                       const std::string& value,
                       const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"lset", key, index, value}, key, true,
                      GetCommandControl(command_control)));
}

void Sentinel::Lset(const std::string& key, int64_t index,
                    const std::string& value, Callback&& callback,
                    const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"lset", key, index, value}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, true);
}

Request Sentinel::Ltrim(const std::string& key, int64_t start, int64_t stop,
                        const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"ltrim", key, start, stop}, key, true,
                      GetCommandControl(command_control)));
}

void Sentinel::Ltrim(const std::string& key, int64_t start, int64_t stop,
                     Callback&& callback,
                     const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"ltrim", key, start, stop}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, true);
}

Request Sentinel::Mget(const std::vector<std::string>& keys,
                       const CommandControl& command_control) {
  assert(!keys.empty());
  return (MakeRequest(CmdArgs{"mget", keys}, keys[0], false,
                      GetCommandControl(command_control)));
}

void Sentinel::Mget(const std::vector<std::string>& keys, Callback&& callback,
                    const CommandControl& command_control) {
  assert(!keys.empty());
  AsyncCommand(
      PrepareCommand(CmdArgs{"mget", keys}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      keys[0], false);
}

Request Sentinel::Persist(const std::string& key,
                          const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"persist", key}, key, true,
                      GetCommandControl(command_control)));
}

void Sentinel::Persist(const std::string& key, Callback&& callback,
                       const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"persist", key}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, true);
}

Request Sentinel::Pexpire(const std::string& key,
                          std::chrono::milliseconds milliseconds,
                          const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"pexpire", key, milliseconds.count()}, key, true,
                      GetCommandControl(command_control)));
}

void Sentinel::Pexpire(const std::string& key,
                       std::chrono::milliseconds milliseconds,
                       Callback&& callback,
                       const CommandControl& command_control) {
  AsyncCommand(PrepareCommand(CmdArgs{"pexpire", key, milliseconds.count()},
                              std::move(callback), task_processor_,
                              GetCommandControl(command_control)),
               key, true);
}

Request Sentinel::Pexpireat(const std::string& key,
                            int64_t milliseconds_timestamp,
                            const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"pexpireat", key, milliseconds_timestamp}, key,
                      true, GetCommandControl(command_control)));
}

void Sentinel::Pexpireat(const std::string& key, int64_t milliseconds_timestamp,
                         Callback&& callback,
                         const CommandControl& command_control) {
  AsyncCommand(PrepareCommand(CmdArgs{"pexpireat", key, milliseconds_timestamp},
                              std::move(callback), task_processor_,
                              GetCommandControl(command_control)),
               key, true);
}

Request Sentinel::Pfadd(const std::string& key, const std::string& element,
                        const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"pfadd", key, element}, key, true,
                      GetCommandControl(command_control)));
}

void Sentinel::Pfadd(const std::string& key, const std::string& element,
                     Callback&& callback,
                     const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"pfadd", key, element}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, true);
}

Request Sentinel::Pfadd(const std::string& key,
                        const std::vector<std::string>& elements,
                        const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"pfadd", key, elements}, key, true,
                      GetCommandControl(command_control)));
}

void Sentinel::Pfadd(const std::string& key,
                     const std::vector<std::string>& elements,
                     Callback&& callback,
                     const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"pfadd", key, elements}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, true);
}

Request Sentinel::Pfcount(const std::string& key,
                          const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"pfcount", key}, key, false,
                      GetCommandControl(command_control)));
}

void Sentinel::Pfcount(const std::string& key, Callback&& callback,
                       const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"pfcount", key}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, false);
}

Request Sentinel::Pfcount(const std::vector<std::string>& keys,
                          const CommandControl& command_control) {
  assert(!keys.empty());
  return (MakeRequest(CmdArgs{"pfcount", keys}, keys[0], false,
                      GetCommandControl(command_control)));
}

void Sentinel::Pfcount(const std::vector<std::string>& keys,
                       Callback&& callback,
                       const CommandControl& command_control) {
  assert(!keys.empty());
  AsyncCommand(
      PrepareCommand(CmdArgs{"pfcount", keys}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      keys[0], false);
}

Request Sentinel::Pfmerge(const std::string& key,
                          const std::vector<std::string>& sourcekeys,
                          const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"pfmerge", key, sourcekeys}, key, true,
                      GetCommandControl(command_control)));
}

void Sentinel::Pfmerge(const std::string& key,
                       const std::vector<std::string>& sourcekeys,
                       Callback&& callback,
                       const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"pfmerge", key, sourcekeys}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, true);
}

Request Sentinel::Psetex(const std::string& key,
                         std::chrono::milliseconds milliseconds,
                         const std::string& val,
                         const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"psetex", key, milliseconds.count(), val}, key,
                      true, GetCommandControl(command_control)));
}

void Sentinel::Psetex(const std::string& key,
                      std::chrono::milliseconds milliseconds,
                      const std::string& val, Callback&& callback,
                      const CommandControl& command_control) {
  AsyncCommand(PrepareCommand(CmdArgs{"psetex", key, milliseconds.count(), val},
                              std::move(callback), task_processor_,
                              GetCommandControl(command_control)),
               key, true);
}

Request Sentinel::Pttl(const std::string& key,
                       const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"pttl", key}, key, false,
                      GetCommandControl(command_control)));
}

void Sentinel::Pttl(const std::string& key, Callback&& callback,
                    const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"pttl", key}, std::move(callback), task_processor_,
                     GetCommandControl(command_control)),
      key, false);
}

Request Sentinel::Restore(const std::string& key, int64_t ttl,
                          const std::string& serialized_value,
                          const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"restore", key, ttl, serialized_value}, key, true,
                      GetCommandControl(command_control)));
}

void Sentinel::Restore(const std::string& key, int64_t ttl,
                       const std::string& serialized_value, Callback&& callback,
                       const CommandControl& command_control) {
  AsyncCommand(PrepareCommand(CmdArgs{"restore", key, ttl, serialized_value},
                              std::move(callback), task_processor_,
                              GetCommandControl(command_control)),
               key, true);
}

Request Sentinel::Restore(const std::string& key, int64_t ttl,
                          const std::string& serialized_value,
                          const std::string& replace,
                          const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"restore", key, ttl, serialized_value, replace},
                      key, true, GetCommandControl(command_control)));
}

void Sentinel::Restore(const std::string& key, int64_t ttl,
                       const std::string& serialized_value,
                       const std::string& replace, Callback&& callback,
                       const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"restore", key, ttl, serialized_value, replace},
                     std::move(callback), task_processor_,
                     GetCommandControl(command_control)),
      key, true);
}

Request Sentinel::Rpop(const std::string& key,
                       const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"rpop", key}, key, true,
                      GetCommandControl(command_control)));
}

void Sentinel::Rpop(const std::string& key, Callback&& callback,
                    const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"rpop", key}, std::move(callback), task_processor_,
                     GetCommandControl(command_control)),
      key, true);
}

Request Sentinel::Rpoplpush(const std::string& key,
                            const std::string& destination,
                            const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"rpoplpush", key, destination}, key, true,
                      GetCommandControl(command_control)));
}

void Sentinel::Rpoplpush(const std::string& key, const std::string& destination,
                         Callback&& callback,
                         const CommandControl& command_control) {
  AsyncCommand(PrepareCommand(CmdArgs{"rpoplpush", key, destination},
                              std::move(callback), task_processor_,
                              GetCommandControl(command_control)),
               key, true);
}

Request Sentinel::Rpush(const std::string& key, const std::string& value,
                        const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"rpush", key, value}, key, true,
                      GetCommandControl(command_control)));
}

void Sentinel::Rpush(const std::string& key, const std::string& value,
                     Callback&& callback,
                     const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"rpush", key, value}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, true);
}

Request Sentinel::Rpush(const std::string& key,
                        const std::vector<std::string>& values,
                        const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"rpush", key, values}, key, true,
                      GetCommandControl(command_control)));
}

void Sentinel::Rpush(const std::string& key,
                     const std::vector<std::string>& values,
                     Callback&& callback,
                     const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"rpush", key, values}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, true);
}

Request Sentinel::Rpushx(const std::string& key, const std::string& value,
                         const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"rpushx", key, value}, key, true,
                      GetCommandControl(command_control)));
}

void Sentinel::Rpushx(const std::string& key, const std::string& value,
                      Callback&& callback,
                      const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"rpushx", key, value}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, true);
}

Request Sentinel::Sadd(const std::string& key, const std::string& member,
                       const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"sadd", key, member}, key, true,
                      GetCommandControl(command_control)));
}

void Sentinel::Sadd(const std::string& key, const std::string& member,
                    Callback&& callback,
                    const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"sadd", key, member}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, true);
}

Request Sentinel::Sadd(const std::string& key,
                       const std::vector<std::string>& members,
                       const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"sadd", key, members}, key, true,
                      GetCommandControl(command_control)));
}

void Sentinel::Sadd(const std::string& key,
                    const std::vector<std::string>& members,
                    Callback&& callback,
                    const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"sadd", key, members}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, true);
}

Request Sentinel::Scard(const std::string& key,
                        const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"scard", key}, key, false,
                      GetCommandControl(command_control)));
}

void Sentinel::Scard(const std::string& key, Callback&& callback,
                     const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"scard", key}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, false);
}

Request Sentinel::Sdiff(const std::vector<std::string>& keys,
                        const CommandControl& command_control) {
  assert(!keys.empty());
  return (MakeRequest(CmdArgs{"sdiff", keys}, keys[0], false,
                      GetCommandControl(command_control)));
}

void Sentinel::Sdiff(const std::vector<std::string>& keys, Callback&& callback,
                     const CommandControl& command_control) {
  assert(!keys.empty());
  AsyncCommand(
      PrepareCommand(CmdArgs{"sdiff", keys}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      keys[0], false);
}

Request Sentinel::Sdiffstore(const std::string& destination,
                             const std::vector<std::string>& keys,
                             const CommandControl& command_control) {
  assert(!keys.empty());
  return (MakeRequest(CmdArgs{"sdiffstore", destination, keys}, keys[0], true,
                      GetCommandControl(command_control)));
}

void Sentinel::Sdiffstore(const std::string& destination,
                          const std::vector<std::string>& keys,
                          Callback&& callback,
                          const CommandControl& command_control) {
  assert(!keys.empty());
  AsyncCommand(PrepareCommand(CmdArgs{"sdiffstore", destination, keys},
                              std::move(callback), task_processor_,
                              GetCommandControl(command_control)),
               keys[0], true);
}

Request Sentinel::Set(const std::string& key, const std::string& value,
                      const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"set", key, value}, key, true,
                      GetCommandControl(command_control)));
}

void Sentinel::Set(const std::string& key, const std::string& value,
                   Callback&& callback, const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"set", key, value}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, true);
}

Request Sentinel::Set(const std::string& key, const std::string& value,
                      const SetOptions& options,
                      const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"set", key, value, options}, key, true,
                      GetCommandControl(command_control)));
}

void Sentinel::Set(const std::string& key, const std::string& value,
                   const SetOptions& options, Callback&& callback,
                   const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"set", key, value, options}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, true);
}

Request Sentinel::Setbit(const std::string& key, int64_t offset,
                         const std::string& value,
                         const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"setbit", key, offset, value}, key, true,
                      GetCommandControl(command_control)));
}

void Sentinel::Setbit(const std::string& key, int64_t offset,
                      const std::string& value, Callback&& callback,
                      const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"setbit", key, offset, value}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, true);
}

Request Sentinel::Setex(const std::string& key, std::chrono::seconds seconds,
                        const std::string& value,
                        const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"setex", key, seconds.count(), value}, key, true,
                      GetCommandControl(command_control)));
}

void Sentinel::Setex(const std::string& key, std::chrono::seconds seconds,
                     const std::string& value, Callback&& callback,
                     const CommandControl& command_control) {
  AsyncCommand(PrepareCommand(CmdArgs{"setex", key, seconds.count(), value},
                              std::move(callback), task_processor_,
                              GetCommandControl(command_control)),
               key, true);
}

Request Sentinel::Setnx(const std::string& key, const std::string& value,
                        const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"setnx", key, value}, key, true,
                      GetCommandControl(command_control)));
}

void Sentinel::Setnx(const std::string& key, const std::string& value,
                     Callback&& callback,
                     const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"setnx", key, value}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, true);
}

Request Sentinel::Setrange(const std::string& key, int64_t offset,
                           const std::string& value,
                           const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"setrange", key, offset, value}, key, true,
                      GetCommandControl(command_control)));
}

void Sentinel::Setrange(const std::string& key, int64_t offset,
                        const std::string& value, Callback&& callback,
                        const CommandControl& command_control) {
  AsyncCommand(PrepareCommand(CmdArgs{"setrange", key, offset, value},
                              std::move(callback), task_processor_,
                              GetCommandControl(command_control)),
               key, true);
}

Request Sentinel::Sinter(const std::vector<std::string>& keys,
                         const CommandControl& command_control) {
  assert(!keys.empty());
  return (MakeRequest(CmdArgs{"sinter", keys}, keys[0], false,
                      GetCommandControl(command_control)));
}

void Sentinel::Sinter(const std::vector<std::string>& keys, Callback&& callback,
                      const CommandControl& command_control) {
  assert(!keys.empty());
  AsyncCommand(
      PrepareCommand(CmdArgs{"sinter", keys}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      keys[0], false);
}

Request Sentinel::Sinterstore(const std::string& destination,
                              const std::vector<std::string>& keys,
                              const CommandControl& command_control) {
  assert(!keys.empty());
  return (MakeRequest(CmdArgs{"sinterstore", destination, keys}, keys[0], true,
                      GetCommandControl(command_control)));
}

void Sentinel::Sinterstore(const std::string& destination,
                           const std::vector<std::string>& keys,
                           Callback&& callback,
                           const CommandControl& command_control) {
  assert(!keys.empty());
  AsyncCommand(PrepareCommand(CmdArgs{"sinterstore", destination, keys},
                              std::move(callback), task_processor_,
                              GetCommandControl(command_control)),
               keys[0], true);
}

Request Sentinel::Sismember(const std::string& key, const std::string& member,
                            const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"sismember", key, member}, key, false,
                      GetCommandControl(command_control)));
}

void Sentinel::Sismember(const std::string& key, const std::string& member,
                         Callback&& callback,
                         const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"sismember", key, member}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, false);
}

Request Sentinel::Smembers(const std::string& key,
                           const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"smembers", key}, key, false,
                      GetCommandControl(command_control)));
}

void Sentinel::Smembers(const std::string& key, Callback&& callback,
                        const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"smembers", key}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, false);
}

Request Sentinel::Smove(const std::string& key, const std::string& destination,
                        const std::string& member,
                        const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"smove", key, destination, member}, key, true,
                      GetCommandControl(command_control)));
}

void Sentinel::Smove(const std::string& key, const std::string& destination,
                     const std::string& member, Callback&& callback,
                     const CommandControl& command_control) {
  AsyncCommand(PrepareCommand(CmdArgs{"smove", key, destination, member},
                              std::move(callback), task_processor_,
                              GetCommandControl(command_control)),
               key, true);
}

Request Sentinel::Spop(const std::string& key,
                       const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"spop", key}, key, true,
                      GetCommandControl(command_control)));
}

void Sentinel::Spop(const std::string& key, Callback&& callback,
                    const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"spop", key}, std::move(callback), task_processor_,
                     GetCommandControl(command_control)),
      key, true);
}

Request Sentinel::Spop(const std::string& key, int64_t count,
                       const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"spop", key, count}, key, true,
                      GetCommandControl(command_control)));
}

void Sentinel::Spop(const std::string& key, int64_t count, Callback&& callback,
                    const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"spop", key, count}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, true);
}

Request Sentinel::Srandmember(const std::string& key,
                              const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"srandmember", key}, key, false,
                      GetCommandControl(command_control)));
}

void Sentinel::Srandmember(const std::string& key, Callback&& callback,
                           const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"srandmember", key}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, false);
}

Request Sentinel::Srandmember(const std::string& key, int64_t count,
                              const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"srandmember", key, count}, key, false,
                      GetCommandControl(command_control)));
}

void Sentinel::Srandmember(const std::string& key, int64_t count,
                           Callback&& callback,
                           const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"srandmember", key, count}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, false);
}

Request Sentinel::Srem(const std::string& key, const std::string& members,
                       const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"srem", key, members}, key, true,
                      GetCommandControl(command_control)));
}

void Sentinel::Srem(const std::string& key, const std::string& members,
                    Callback&& callback,
                    const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"srem", key, members}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, true);
}

Request Sentinel::Srem(const std::string& key,
                       const std::vector<std::string>& members,
                       const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"srem", key, members}, key, true,
                      GetCommandControl(command_control)));
}

void Sentinel::Srem(const std::string& key,
                    const std::vector<std::string>& members,
                    Callback&& callback,
                    const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"srem", key, members}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, true);
}

Request Sentinel::Strlen(const std::string& key,
                         const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"strlen", key}, key, false,
                      GetCommandControl(command_control)));
}

void Sentinel::Strlen(const std::string& key, Callback&& callback,
                      const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"strlen", key}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, false);
}

Request Sentinel::Sunion(const std::vector<std::string>& keys,
                         const CommandControl& command_control) {
  assert(!keys.empty());
  return (MakeRequest(CmdArgs{"sunion", keys}, keys[0], false,
                      GetCommandControl(command_control)));
}

void Sentinel::Sunion(const std::vector<std::string>& keys, Callback&& callback,
                      const CommandControl& command_control) {
  assert(!keys.empty());
  AsyncCommand(
      PrepareCommand(CmdArgs{"sunion", keys}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      keys[0], false);
}

Request Sentinel::Sunionstore(const std::string& destination,
                              const std::vector<std::string>& keys,
                              const CommandControl& command_control) {
  assert(!keys.empty());
  return (MakeRequest(CmdArgs{"sunionstore", destination, keys}, keys[0], true,
                      GetCommandControl(command_control)));
}

void Sentinel::Sunionstore(const std::string& destination,
                           const std::vector<std::string>& keys,
                           Callback&& callback,
                           const CommandControl& command_control) {
  assert(!keys.empty());
  AsyncCommand(PrepareCommand(CmdArgs{"sunionstore", destination, keys},
                              std::move(callback), task_processor_,
                              GetCommandControl(command_control)),
               keys[0], true);
}

Request Sentinel::Ttl(const std::string& key,
                      const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"ttl", key}, key, false,
                      GetCommandControl(command_control)));
}

void Sentinel::Ttl(const std::string& key, Callback&& callback,
                   const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"ttl", key}, std::move(callback), task_processor_,
                     GetCommandControl(command_control)),
      key, false);
}

Request Sentinel::Type(const std::string& key,
                       const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"type", key}, key, false,
                      GetCommandControl(command_control)));
}

void Sentinel::Type(const std::string& key, Callback&& callback,
                    const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"type", key}, std::move(callback), task_processor_,
                     GetCommandControl(command_control)),
      key, false);
}

Request Sentinel::Zadd(const std::string& key, double score,
                       const std::string& member,
                       const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"zadd", key, score, member}, key, true,
                      GetCommandControl(command_control)));
}

void Sentinel::Zadd(const std::string& key, double score,
                    const std::string& member, Callback&& callback,
                    const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"zadd", key, score, member}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, true);
}

Request Sentinel::Zcard(const std::string& key,
                        const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"zcard", key}, key, false,
                      GetCommandControl(command_control)));
}

void Sentinel::Zcard(const std::string& key, Callback&& callback,
                     const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"zcard", key}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, false);
}

Request Sentinel::Zcount(const std::string& key, int64_t min, int64_t max,
                         const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"zcount", key, min, max}, key, false,
                      GetCommandControl(command_control)));
}

void Sentinel::Zcount(const std::string& key, int64_t min, int64_t max,
                      Callback&& callback,
                      const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"zcount", key, min, max}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, false);
}

Request Sentinel::Zcount(const std::string& key, double min, double max,
                         const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"zcount", key, min, max}, key, false,
                      GetCommandControl(command_control)));
}

void Sentinel::Zcount(const std::string& key, double min, double max,
                      Callback&& callback,
                      const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"zcount", key, min, max}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, false);
}

Request Sentinel::Zcount(const std::string& key, const std::string& min,
                         const std::string& max,
                         const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"zcount", key, min, max}, key, false,
                      GetCommandControl(command_control)));
}

void Sentinel::Zcount(const std::string& key, const std::string& min,
                      const std::string& max, Callback&& callback,
                      const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"zcount", key, min, max}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, false);
}

Request Sentinel::Zincrby(const std::string& key, int64_t incr,
                          const std::string& member,
                          const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"zincrby", key, incr, member}, key, true,
                      GetCommandControl(command_control)));
}

void Sentinel::Zincrby(const std::string& key, int64_t incr,
                       const std::string& member, Callback&& callback,
                       const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"zincrby", key, incr, member}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, true);
}

Request Sentinel::Zincrby(const std::string& key, double incr,
                          const std::string& member,
                          const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"zincrby", key, incr, member}, key, true,
                      GetCommandControl(command_control)));
}

void Sentinel::Zincrby(const std::string& key, double incr,
                       const std::string& member, Callback&& callback,
                       const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"zincrby", key, incr, member}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, true);
}

Request Sentinel::Zincrby(const std::string& key, const std::string& incr,
                          const std::string& member,
                          const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"zincrby", key, incr, member}, key, true,
                      GetCommandControl(command_control)));
}

void Sentinel::Zincrby(const std::string& key, const std::string& incr,
                       const std::string& member, Callback&& callback,
                       const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"zincrby", key, incr, member}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, true);
}

Request Sentinel::Zlexcount(const std::string& key, int64_t min, int64_t max,
                            const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"zlexcount", key, min, max}, key, false,
                      GetCommandControl(command_control)));
}

void Sentinel::Zlexcount(const std::string& key, int64_t min, int64_t max,
                         Callback&& callback,
                         const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"zlexcount", key, min, max}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, false);
}

Request Sentinel::Zlexcount(const std::string& key, double min, double max,
                            const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"zlexcount", key, min, max}, key, false,
                      GetCommandControl(command_control)));
}

void Sentinel::Zlexcount(const std::string& key, double min, double max,
                         Callback&& callback,
                         const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"zlexcount", key, min, max}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, false);
}

Request Sentinel::Zlexcount(const std::string& key, const std::string& min,
                            const std::string& max,
                            const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"zlexcount", key, min, max}, key, false,
                      GetCommandControl(command_control)));
}

void Sentinel::Zlexcount(const std::string& key, const std::string& min,
                         const std::string& max, Callback&& callback,
                         const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"zlexcount", key, min, max}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, false);
}

Request Sentinel::Zrange(const std::string& key, int64_t start, int64_t stop,
                         const ScoreOptions& score_options,
                         const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"zrange", key, start, stop, score_options}, key,
                      false, GetCommandControl(command_control)));
}

void Sentinel::Zrange(const std::string& key, int64_t start, int64_t stop,
                      const ScoreOptions& score_options, Callback&& callback,
                      const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"zrange", key, start, stop, score_options},
                     std::move(callback), task_processor_,
                     GetCommandControl(command_control)),
      key, false);
}

Request Sentinel::Zrange(const std::string& key, double start, double stop,
                         const ScoreOptions& score_options,
                         const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"zrange", key, start, stop, score_options}, key,
                      false, GetCommandControl(command_control)));
}

void Sentinel::Zrange(const std::string& key, double start, double stop,
                      const ScoreOptions& score_options, Callback&& callback,
                      const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"zrange", key, start, stop, score_options},
                     std::move(callback), task_processor_,
                     GetCommandControl(command_control)),
      key, false);
}

Request Sentinel::Zrange(const std::string& key, const std::string& start,
                         const std::string& stop,
                         const ScoreOptions& score_options,
                         const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"zrange", key, start, stop, score_options}, key,
                      false, GetCommandControl(command_control)));
}

void Sentinel::Zrange(const std::string& key, const std::string& start,
                      const std::string& stop,
                      const ScoreOptions& score_options, Callback&& callback,
                      const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"zrange", key, start, stop, score_options},
                     std::move(callback), task_processor_,
                     GetCommandControl(command_control)),
      key, false);
}

Request Sentinel::Zrank(const std::string& key, const std::string& member,
                        const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"zrank", key, member}, key, false,
                      GetCommandControl(command_control)));
}

void Sentinel::Zrank(const std::string& key, const std::string& member,
                     Callback&& callback,
                     const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"zrank", key, member}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, false);
}

Request Sentinel::Zrem(const std::string& key, const std::string& member,
                       const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"zrem", key, member}, key, true,
                      GetCommandControl(command_control)));
}

void Sentinel::Zrem(const std::string& key, const std::string& member,
                    Callback&& callback,
                    const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"zrem", key, member}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, true);
}

Request Sentinel::Zrem(const std::string& key,
                       const std::vector<std::string>& members,
                       const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"zrem", key, members}, key, true,
                      GetCommandControl(command_control)));
}

void Sentinel::Zrem(const std::string& key,
                    const std::vector<std::string>& members,
                    Callback&& callback,
                    const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"zrem", key, members}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, true);
}

Request Sentinel::Zremrangebylex(const std::string& key, int64_t min,
                                 int64_t max,
                                 const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"zremrangebylex", key, min, max}, key, true,
                      GetCommandControl(command_control)));
}

void Sentinel::Zremrangebylex(const std::string& key, int64_t min, int64_t max,
                              Callback&& callback,
                              const CommandControl& command_control) {
  AsyncCommand(PrepareCommand(CmdArgs{"zremrangebylex", key, min, max},
                              std::move(callback), task_processor_,
                              GetCommandControl(command_control)),
               key, true);
}

Request Sentinel::Zremrangebylex(const std::string& key, double min, double max,
                                 const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"zremrangebylex", key, min, max}, key, true,
                      GetCommandControl(command_control)));
}

void Sentinel::Zremrangebylex(const std::string& key, double min, double max,
                              Callback&& callback,
                              const CommandControl& command_control) {
  AsyncCommand(PrepareCommand(CmdArgs{"zremrangebylex", key, min, max},
                              std::move(callback), task_processor_,
                              GetCommandControl(command_control)),
               key, true);
}

Request Sentinel::Zremrangebylex(const std::string& key, const std::string& min,
                                 const std::string& max,
                                 const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"zremrangebylex", key, min, max}, key, true,
                      GetCommandControl(command_control)));
}

void Sentinel::Zremrangebylex(const std::string& key, const std::string& min,
                              const std::string& max, Callback&& callback,
                              const CommandControl& command_control) {
  AsyncCommand(PrepareCommand(CmdArgs{"zremrangebylex", key, min, max},
                              std::move(callback), task_processor_,
                              GetCommandControl(command_control)),
               key, true);
}

Request Sentinel::Zremrangebyrank(const std::string& key, int64_t start,
                                  int64_t stop,
                                  const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"zremrangebyrank", key, start, stop}, key, true,
                      GetCommandControl(command_control)));
}

void Sentinel::Zremrangebyrank(const std::string& key, int64_t start,
                               int64_t stop, Callback&& callback,
                               const CommandControl& command_control) {
  AsyncCommand(PrepareCommand(CmdArgs{"zremrangebyrank", key, start, stop},
                              std::move(callback), task_processor_,
                              GetCommandControl(command_control)),
               key, true);
}

Request Sentinel::Zremrangebyrank(const std::string& key, double start,
                                  double stop,
                                  const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"zremrangebyrank", key, start, stop}, key, true,
                      GetCommandControl(command_control)));
}

void Sentinel::Zremrangebyrank(const std::string& key, double start,
                               double stop, Callback&& callback,
                               const CommandControl& command_control) {
  AsyncCommand(PrepareCommand(CmdArgs{"zremrangebyrank", key, start, stop},
                              std::move(callback), task_processor_,
                              GetCommandControl(command_control)),
               key, true);
}

Request Sentinel::Zremrangebyrank(const std::string& key,
                                  const std::string& start,
                                  const std::string& stop,
                                  const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"zremrangebyrank", key, start, stop}, key, true,
                      GetCommandControl(command_control)));
}

void Sentinel::Zremrangebyrank(const std::string& key, const std::string& start,
                               const std::string& stop, Callback&& callback,
                               const CommandControl& command_control) {
  AsyncCommand(PrepareCommand(CmdArgs{"zremrangebyrank", key, start, stop},
                              std::move(callback), task_processor_,
                              GetCommandControl(command_control)),
               key, true);
}

Request Sentinel::Zremrangebyscore(const std::string& key, int64_t min,
                                   int64_t max,
                                   const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"zremrangebyscore", key, min, max}, key, true,
                      GetCommandControl(command_control)));
}

void Sentinel::Zremrangebyscore(const std::string& key, int64_t min,
                                int64_t max, Callback&& callback,
                                const CommandControl& command_control) {
  AsyncCommand(PrepareCommand(CmdArgs{"zremrangebyscore", key, min, max},
                              std::move(callback), task_processor_,
                              GetCommandControl(command_control)),
               key, true);
}

Request Sentinel::Zremrangebyscore(const std::string& key, double min,
                                   double max,
                                   const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"zremrangebyscore", key, min, max}, key, true,
                      GetCommandControl(command_control)));
}

void Sentinel::Zremrangebyscore(const std::string& key, double min, double max,
                                Callback&& callback,
                                const CommandControl& command_control) {
  AsyncCommand(PrepareCommand(CmdArgs{"zremrangebyscore", key, min, max},
                              std::move(callback), task_processor_,
                              GetCommandControl(command_control)),
               key, true);
}

Request Sentinel::Zremrangebyscore(const std::string& key,
                                   const std::string& min,
                                   const std::string& max,
                                   const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"zremrangebyscore", key, min, max}, key, true,
                      GetCommandControl(command_control)));
}

void Sentinel::Zremrangebyscore(const std::string& key, const std::string& min,
                                const std::string& max, Callback&& callback,
                                const CommandControl& command_control) {
  AsyncCommand(PrepareCommand(CmdArgs{"zremrangebyscore", key, min, max},
                              std::move(callback), task_processor_,
                              GetCommandControl(command_control)),
               key, true);
}

Request Sentinel::Zrevrange(const std::string& key, int64_t start, int64_t stop,
                            const ScoreOptions& score_options,
                            const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"zrevrange", key, start, stop, score_options},
                      key, false, GetCommandControl(command_control)));
}

void Sentinel::Zrevrange(const std::string& key, int64_t start, int64_t stop,
                         const ScoreOptions& score_options, Callback&& callback,
                         const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"zrevrange", key, start, stop, score_options},
                     std::move(callback), task_processor_,
                     GetCommandControl(command_control)),
      key, false);
}

Request Sentinel::Zrevrange(const std::string& key, double start, double stop,
                            const ScoreOptions& score_options,
                            const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"zrevrange", key, start, stop, score_options},
                      key, false, GetCommandControl(command_control)));
}

void Sentinel::Zrevrange(const std::string& key, double start, double stop,
                         const ScoreOptions& score_options, Callback&& callback,
                         const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"zrevrange", key, start, stop, score_options},
                     std::move(callback), task_processor_,
                     GetCommandControl(command_control)),
      key, false);
}

Request Sentinel::Zrevrange(const std::string& key, const std::string& start,
                            const std::string& stop,
                            const ScoreOptions& score_options,
                            const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"zrevrange", key, start, stop, score_options},
                      key, false, GetCommandControl(command_control)));
}

void Sentinel::Zrevrange(const std::string& key, const std::string& start,
                         const std::string& stop,
                         const ScoreOptions& score_options, Callback&& callback,
                         const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"zrevrange", key, start, stop, score_options},
                     std::move(callback), task_processor_,
                     GetCommandControl(command_control)),
      key, false);
}

Request Sentinel::Zrevrank(const std::string& key, const std::string& member,
                           const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"zrevrank", key, member}, key, false,
                      GetCommandControl(command_control)));
}

void Sentinel::Zrevrank(const std::string& key, const std::string& member,
                        Callback&& callback,
                        const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"zrevrank", key, member}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, false);
}

Request Sentinel::Zscore(const std::string& key, const std::string& member,
                         const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"zscore", key, member}, key, false,
                      GetCommandControl(command_control)));
}

void Sentinel::Zscore(const std::string& key, const std::string& member,
                      Callback&& callback,
                      const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"zscore", key, member}, std::move(callback),
                     task_processor_, GetCommandControl(command_control)),
      key, false);
}

Request Sentinel::Randomkey(size_t shard,
                            const CommandControl& command_control) {
  return (MakeRequest(CmdArgs{"randomkey"}, shard, false,
                      GetCommandControl(command_control)));
}

void Sentinel::Randomkey(size_t shard, Callback&& callback,
                         const CommandControl& command_control) {
  AsyncCommand(
      PrepareCommand(CmdArgs{"randomkey"}, std::move(callback), task_processor_,
                     GetCommandControl(command_control)),
      false, shard);
}

Request Sentinel::Publish(
    const std::string& channel, const std::string& message,
    const CommandControl& command_control /* = CommandControl()*/) {
  return (MakeRequest(CmdArgs{"PUBLISH", channel, message}, "", true,
                      command_control));
}

void Sentinel::Subscribe(
    const std::string& channel, const MessageCallback& message_callback,
    const SubscribeCallback&
        subscribe_callback /* = default_subscribe_callback*/,
    const UnsubscribeCallback&
        unsubscribe_callback /* = default_unsubscribe_callback*/) {
  AsyncCommand(PrepareCommand(CmdArgs{"SUBSCRIBE", channel},
                              [message_callback, subscribe_callback,
                               unsubscribe_callback](redis::ReplyPtr reply) {
                                OnSubscribeReply(message_callback,
                                                 subscribe_callback,
                                                 unsubscribe_callback, reply);
                              }));
}

void Sentinel::Subscribe(
    const std::vector<std::string>& channels,
    const MessageCallback& message_callback,
    const SubscribeCallback&
        subscribe_callback /* = default_subscribe_callback*/,
    const UnsubscribeCallback&
        unsubscribe_callback /* = default_unsubscribe_callback*/) {
  AsyncCommand(PrepareCommand(CmdArgs{"SUBSCRIBE", channels},
                              [message_callback, subscribe_callback,
                               unsubscribe_callback](redis::ReplyPtr reply) {
                                OnSubscribeReply(message_callback,
                                                 subscribe_callback,
                                                 unsubscribe_callback, reply);
                              }));
}

void Sentinel::Psubscribe(
    const std::string& channel, const PmessageCallback& pmessage_callback,
    const SubscribeCallback&
        subscribe_callback /* = default_subscribe_callback*/,
    const UnsubscribeCallback&
        unsubscribe_callback /* = default_unsubscribe_callback*/) {
  AsyncCommand(PrepareCommand(CmdArgs{"PSUBSCRIBE", channel},
                              [pmessage_callback, subscribe_callback,
                               unsubscribe_callback](redis::ReplyPtr reply) {
                                OnPsubscribeReply(pmessage_callback,
                                                  subscribe_callback,
                                                  unsubscribe_callback, reply);
                              }));
}

void Sentinel::Psubscribe(
    const std::vector<std::string>& channels,
    const PmessageCallback& pmessage_callback,
    const SubscribeCallback&
        subscribe_callback /* = default_subscribe_callback*/,
    const UnsubscribeCallback&
        unsubscribe_callback /* = default_unsubscribe_callback*/) {
  AsyncCommand(PrepareCommand(CmdArgs{"PSUBSCRIBE", channels},
                              [pmessage_callback, subscribe_callback,
                               unsubscribe_callback](redis::ReplyPtr reply) {
                                OnPsubscribeReply(pmessage_callback,
                                                  subscribe_callback,
                                                  unsubscribe_callback, reply);
                              }));
}

void Sentinel::Unsubscribe(const std::string& channel) {
  AsyncCommand(PrepareCommand(CmdArgs{"UNSUBSCRIBE", channel}, nullptr));
}

void Sentinel::Unsubscribe(const std::vector<std::string>& channels) {
  AsyncCommand(PrepareCommand(CmdArgs{"UNSUBSCRIBE", channels}, nullptr));
}

void Sentinel::Punsubscribe(const std::string& channel) {
  AsyncCommand(PrepareCommand(CmdArgs{"PUNSUBSCRIBE", channel}, nullptr));
}

void Sentinel::Punsubscribe(const std::vector<std::string>& channels) {
  AsyncCommand(PrepareCommand(CmdArgs{"PUNSUBSCRIBE", channels}, nullptr));
}

const Sentinel::SubscribeCallback Sentinel::default_subscribe_callback =
    Sentinel::SubscribeCallbackNone;
const Sentinel::UnsubscribeCallback Sentinel::default_unsubscribe_callback =
    Sentinel::UnsubscribeCallbackNone;

Transaction Sentinel::Multi() { return Transaction(shared_from_this()); }

void Sentinel::OnSubscribeReply(const MessageCallback message_callback,
                                const SubscribeCallback subscribe_callback,
                                const UnsubscribeCallback unsubscribe_callback,
                                redis::ReplyPtr reply_ptr) {
  LOG_TRACE() << reply_ptr->data.ToString();
  if (!reply_ptr->data.IsArray()) {
    LOG_WARNING() << "strange response to subscribe command: "
                  << reply_ptr->data.ToString();
    return;
  }
  const auto& reply = reply_ptr->data.GetArray();
  if (reply.size() != 3 || !reply[0].IsString()) return;
  if (!strcasecmp(reply[0].GetString().c_str(), "SUBSCRIBE")) {
    if (subscribe_callback)
      subscribe_callback(reply[1].GetString(), reply[2].GetInt());
  } else if (!strcasecmp(reply[0].GetString().c_str(), "UNSUBSCRIBE")) {
    if (unsubscribe_callback)
      unsubscribe_callback(reply[1].GetString(), reply[2].GetInt());
  } else if (!strcasecmp(reply[0].GetString().c_str(), "MESSAGE")) {
    if (message_callback)
      message_callback(reply[1].GetString(), reply[2].GetString());
  }
}

void Sentinel::OnPsubscribeReply(const PmessageCallback pmessage_callback,
                                 const SubscribeCallback subscribe_callback,
                                 const UnsubscribeCallback unsubscribe_callback,
                                 redis::ReplyPtr reply_ptr) {
  LOG_TRACE() << reply_ptr->data.ToString();
  if (!reply_ptr->data.IsArray()) {
    LOG_WARNING() << "strange response to psubscribe command: "
                  << reply_ptr->data.ToString();
    return;
  }
  const auto& reply = reply_ptr->data.GetArray();
  if (!reply[0].IsString()) return;
  if (!strcasecmp(reply[0].GetString().c_str(), "PSUBSCRIBE")) {
    if (reply.size() == 3 && subscribe_callback)
      subscribe_callback(reply[1].GetString(), reply[2].GetInt());
  } else if (!strcasecmp(reply[0].GetString().c_str(), "PUNSUBSCRIBE")) {
    if (reply.size() == 3 && unsubscribe_callback)
      unsubscribe_callback(reply[1].GetString(), reply[2].GetInt());
  } else if (!strcasecmp(reply[0].GetString().c_str(), "PMESSAGE")) {
    if (reply.size() == 4 && pmessage_callback)
      pmessage_callback(reply[1].GetString(), reply[2].GetString(),
                        reply[3].GetString());
  }
}

CommandControl Sentinel::GetCommandControl(const CommandControl& cc) {
  return CommandControl(cc.timeout_single > std::chrono::milliseconds::zero()
                            ? cc.timeout_single
                            : default_command_control_.timeout_single,
                        cc.timeout_all > std::chrono::milliseconds::zero()
                            ? cc.timeout_all
                            : default_command_control_.timeout_all,
                        cc.max_retries > 0
                            ? cc.max_retries
                            : default_command_control_.max_retries);
}

}  // namespace redis
}  // namespace storages
