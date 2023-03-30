#include <storages/redis/impl/sentinel.hpp>

#include <memory>
#include <stdexcept>

#include <userver/logging/log.hpp>

#include <engine/ev/thread_control.hpp>
#include <userver/clients/dns/resolver.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/assert.hpp>

#include <storages/redis/impl/command.hpp>
#include <storages/redis/impl/redis.hpp>
#include <storages/redis/impl/sentinel_impl.hpp>
#include <storages/redis/impl/subscribe_sentinel.hpp>
#include <userver/storages/redis/impl/base.hpp>
#include <userver/storages/redis/impl/exception.hpp>
#include <userver/storages/redis/impl/reply.hpp>

USERVER_NAMESPACE_BEGIN

namespace redis {
namespace {

void ThrowIfCancelled() {
  if (engine::current_task::GetTaskProcessorOptional() &&
      engine::current_task::ShouldCancel()) {
    throw RequestCancelledException(
        "Failed to make redis request due to task cancellation");
  }
}

constexpr std::chrono::milliseconds kResolveTimeout{500};

ConnectionInfo::HostVector ResolveDns(const std::string& host,
                                      clients::dns::Resolver* resolver) {
  if (!resolver) return {};
  ConnectionInfo::HostVector result;
  try {
    auto addrs = resolver->Resolve(
        host, engine::Deadline::FromDuration(kResolveTimeout));
    std::transform(addrs.begin(), addrs.end(), std::back_inserter(result),
                   [](auto addr) { return addr.PrimaryAddressString(); });
  } catch (const std::exception& e) {
    LOG_WARNING() << "exception occurred during dns resolving for host " << host
                  << ": " << e;
  }
  return result;
}

}  // namespace

Sentinel::Sentinel(
    const std::shared_ptr<ThreadPools>& thread_pools,
    const std::vector<std::string>& shards,
    const std::vector<ConnectionInfo>& conns, std::string shard_group_name,
    const std::string& client_name, const Password& password,
    ConnectionSecurity connection_security, ReadyChangeCallback ready_callback,
    std::unique_ptr<KeyShard>&& key_shard, CommandControl command_control,
    const testsuite::RedisControl& testsuite_redis_control, ConnectionMode mode)
    : thread_pools_(thread_pools),
      secdist_default_command_control_(command_control),
      testsuite_redis_control_(testsuite_redis_control) {
  config_default_command_control_.Set(
      std::make_shared<CommandControl>(secdist_default_command_control_));

  if (!thread_pools_) {
    throw std::runtime_error("can't create Sentinel with empty thread_pools");
  }
  sentinel_thread_control_ = std::make_unique<engine::ev::ThreadControl>(
      thread_pools_->GetSentinelThreadPool().NextThread());

  sentinel_thread_control_->RunInEvLoopBlocking([&]() {
    impl_ = std::make_unique<SentinelImpl>(
        *sentinel_thread_control_, thread_pools_->GetRedisThreadPool(), *this,
        shards, conns, std::move(shard_group_name), client_name, password,
        connection_security, std::move(ready_callback), std::move(key_shard),
        mode);
  });
}

Sentinel::~Sentinel() {
  sentinel_thread_control_->RunInEvLoopBlocking([this]() { impl_.reset(); });
  UASSERT(!impl_);
}

void Sentinel::Start() {
  sentinel_thread_control_->RunInEvLoopBlocking([this] { impl_->Start(); });
}

void Sentinel::WaitConnectedDebug(bool allow_empty_slaves) {
  impl_->WaitConnectedDebug(allow_empty_slaves);
}

void Sentinel::WaitConnectedOnce(RedisWaitConnected wait_connected) {
  impl_->WaitConnectedOnce(wait_connected.MergeWith(testsuite_redis_control_));
}

void Sentinel::ForceUpdateHosts() { impl_->ForceUpdateHosts(); }

std::shared_ptr<Sentinel> Sentinel::CreateSentinel(
    const std::shared_ptr<ThreadPools>& thread_pools,
    const secdist::RedisSettings& settings, std::string shard_group_name,
    const std::string& client_name, KeyShardFactory key_shard_factory,
    const CommandControl& command_control,
    const testsuite::RedisControl& testsuite_redis_control,
    clients::dns::Resolver* dns_resolver) {
  auto ready_callback = [](size_t shard, const std::string& shard_name,
                           bool ready) {
    LOG_INFO() << "redis: ready_callback:"
               << "  shard = " << shard << "  shard_name = " << shard_name
               << "  ready = " << (ready ? "true" : "false");
  };
  return CreateSentinel(thread_pools, settings, std::move(shard_group_name),
                        client_name, std::move(ready_callback),
                        std::move(key_shard_factory), command_control,
                        testsuite_redis_control, dns_resolver);
}

std::shared_ptr<Sentinel> Sentinel::CreateSentinel(
    const std::shared_ptr<ThreadPools>& thread_pools,
    const secdist::RedisSettings& settings, std::string shard_group_name,
    const std::string& client_name,
    Sentinel::ReadyChangeCallback ready_callback,
    KeyShardFactory key_shard_factory, const CommandControl& command_control,
    const testsuite::RedisControl& testsuite_redis_control,
    clients::dns::Resolver* dns_resolver) {
  const auto& password = settings.password;

  const std::vector<std::string>& shards = settings.shards;
  LOG_DEBUG() << "shards.size() = " << shards.size();
  for (const std::string& shard : shards)
    LOG_DEBUG() << "shard:  name = " << shard;

  std::vector<redis::ConnectionInfo> conns;
  conns.reserve(settings.sentinels.size());
  LOG_DEBUG() << "sentinels.size() = " << settings.sentinels.size();
  auto key_shard = key_shard_factory(shards.size());
  for (const auto& sentinel : settings.sentinels) {
    LOG_DEBUG() << "sentinel:  host = " << sentinel.host
                << "  port = " << sentinel.port;
    // SENTINEL MASTERS/SLAVES works without auth, sentinel has no AUTH command.
    // CLUSTER SLOTS works after auth only. Masters and slaves used instead of
    // sentinels in cluster mode.
    conns.emplace_back(sentinel.host, sentinel.port,
                       (key_shard ? Password("") : password), false,
                       settings.secure_connection,
                       ResolveDns(sentinel.host, dns_resolver));
  }

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
        password, settings.secure_connection, std::move(ready_callback),
        std::move(key_shard), command_control, testsuite_redis_control);
    client->Start();
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
    if (impl_->IsInClusterMode())
      throw InvalidArgumentException(
          "force_shard_idx is not supported in RedisCluster mode");
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
  size_t shard = 0;
  if (command->control.force_shard_idx) {
    if (impl_->IsInClusterMode())
      throw InvalidArgumentException(
          "force_shard_idx is not supported in RedisCluster mode");
    shard = *command->control.force_shard_idx;
  } else {
    shard = impl_->ShardByKey(key);
  }

  CheckShardIdx(shard);
  try {
    impl_->AsyncCommand(
        {command, master, shard, std::chrono::steady_clock::now()});
  } catch (const std::exception& ex) {
    LOG_WARNING() << "exception in " << __func__ << " '" << ex.what() << "'";
  }
}

void Sentinel::AsyncCommandToSentinel(CommandPtr command) {
  if (!impl_) return;
  ThrowIfCancelled();
  try {
    impl_->AsyncCommandToSentinel(std::move(command));
  } catch (const std::exception& ex) {
    LOG_WARNING() << "exception in " << __func__ << " '" << ex.what() << "'";
  }
}

std::string Sentinel::CreateTmpKey(const std::string& key, std::string prefix) {
  size_t key_start = 0;
  size_t key_len = 0;
  GetRedisKey(key, &key_start, &key_len);

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

SentinelStatistics Sentinel::GetStatistics(
    const MetricsSettings& settings) const {
  return impl_->GetStatistics(settings);
}

void Sentinel::SetCommandsBufferingSettings(
    CommandsBufferingSettings commands_buffering_settings) {
  return impl_->SetCommandsBufferingSettings(commands_buffering_settings);
}

void Sentinel::SetReplicationMonitoringSettings(
    const ReplicationMonitoringSettings& replication_monitoring_settings) {
  impl_->SetReplicationMonitoringSettings(replication_monitoring_settings);
}

std::vector<Request> Sentinel::MakeRequests(
    CmdArgs&& args, bool master, const CommandControl& command_control,
    size_t replies_to_skip) {
  std::vector<Request> rslt;

  for (size_t shard = 0; shard < impl_->ShardsCount(); ++shard) {
    rslt.push_back(MakeRequest(args.Clone(), shard, master, command_control,
                               replies_to_skip));
  }

  return rslt;
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
      .MergeWith(cc)
      .MergeWith(testsuite_redis_control_);
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
                                 const std::string& newkey) const {
  if (ShardByKey(key) != ShardByKey(newkey))
    throw InvalidArgumentException(
        "key and newkey must have the same shard key");
}

}  // namespace redis

USERVER_NAMESPACE_END
