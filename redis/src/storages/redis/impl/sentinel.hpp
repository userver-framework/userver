#pragma once

#include <string>
#include <unordered_map>

#include <boost/signals2.hpp>

#include <userver/clients/dns/resolver_fwd.hpp>
#include <userver/dynamic_config/source.hpp>
#include <userver/utils/swappingsmart.hpp>

#include <userver/storages/redis/impl/base.hpp>
#include <userver/storages/redis/impl/command_options.hpp>
#include <userver/storages/redis/impl/keyshard.hpp>
#include <userver/storages/redis/impl/request.hpp>
#include <userver/storages/redis/impl/secdist_redis.hpp>
#include <userver/storages/redis/impl/thread_pools.hpp>
#include <userver/storages/redis/impl/types.hpp>
#include <userver/storages/redis/impl/wait_connected_mode.hpp>

#include <storages/redis/impl/redis_stats.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::ev {
class ThreadControl;
}  // namespace engine::ev

namespace redis {

// We need only one thread for sentinels different from redis threads
const int kDefaultSentinelThreadPoolSize = 1;

// It works fine with 8 threads in driver_authorizer
const int kDefaultRedisThreadPoolSize = 8;

const auto kSentinelGetHostsCheckInterval = std::chrono::seconds(3);
const auto kProcessWaitingCommandsInterval = std::chrono::seconds(3);

// Forward declarations
class SentinelImplBase;
class SentinelImpl;
class Shard;

class Sentinel {
 public:
  using ReadyChangeCallback = std::function<void(
      size_t shard, const std::string& shard_name, bool ready)>;

  Sentinel(const std::shared_ptr<ThreadPools>& thread_pools,
           const std::vector<std::string>& shards,
           const std::vector<ConnectionInfo>& conns,
           std::string shard_group_name, const std::string& client_name,
           const Password& password, ConnectionSecurity connection_security,
           ReadyChangeCallback ready_callback,
           dynamic_config::Source dynamic_config_source,
           std::unique_ptr<KeyShard>&& key_shard = nullptr,
           CommandControl command_control = kDefaultCommandControl,
           const testsuite::RedisControl& testsuite_redis_control = {},
           ConnectionMode mode = ConnectionMode::kCommands);
  virtual ~Sentinel();

  void Start();

  // Wait until connections to all servers are up
  void WaitConnectedDebug(bool allow_empty_slaves = false);

  // Wait until connections to all shards are up first time.
  // mode == kNoWait: do not wait.
  // mode == kMaster: for each shard need a connection to its master.
  // mode == kSlave: for each shard need a connection to at least one of its
  // slaves.
  // mode == kMasterOrSlave: for each shard need a connection to its master or
  // at least one of its slaves.
  // mode == kMasterAndSlave: for each shard need a connection to its master and
  // at least one of its slaves.
  void WaitConnectedOnce(RedisWaitConnected wait_connected);

  void ForceUpdateHosts();

  static std::shared_ptr<redis::Sentinel> CreateSentinel(
      const std::shared_ptr<ThreadPools>& thread_pools,
      const secdist::RedisSettings& settings, std::string shard_group_name,
      dynamic_config::Source dynamic_config_source,
      const std::string& client_name, KeyShardFactory key_shard_factory,
      const CommandControl& command_control = kDefaultCommandControl,
      const testsuite::RedisControl& testsuite_redis_control = {},
      clients::dns::Resolver* dns_resolver = nullptr);
  static std::shared_ptr<redis::Sentinel> CreateSentinel(
      const std::shared_ptr<ThreadPools>& thread_pools,
      const secdist::RedisSettings& settings, std::string shard_group_name,
      dynamic_config::Source dynamic_config_source,
      const std::string& client_name, ReadyChangeCallback ready_callback,
      KeyShardFactory key_shard_factory,
      const CommandControl& command_control = kDefaultCommandControl,
      const testsuite::RedisControl& testsuite_redis_control = {},
      clients::dns::Resolver* dns_resolver = nullptr);

  void Restart();

  std::unordered_map<ServerId, size_t, ServerIdHasher>
  GetAvailableServersWeighted(size_t shard_idx, bool with_master,
                              const CommandControl& cc = {}) const;

  void AsyncCommand(CommandPtr command, bool master = true, size_t shard = 0);
  void AsyncCommand(CommandPtr command, const std::string& key,
                    bool master = true);
  void AsyncCommandToSentinel(CommandPtr command);

  // return a new temporary key with the same shard index
  static std::string CreateTmpKey(const std::string& key,
                                  std::string prefix = "tmp:");

  size_t ShardByKey(const std::string& key) const;
  size_t ShardsCount() const;
  void CheckShardIdx(size_t shard_idx) const;
  static void CheckShardIdx(size_t shard_idx, size_t shard_count);

  // Returns a non-empty key of the minimum length consisting of lowercase
  // letters for a given shard.
  const std::string& GetAnyKeyForShard(size_t shard_idx) const;

  SentinelStatistics GetStatistics(const MetricsSettings& settings) const;

  void SetCommandsBufferingSettings(
      CommandsBufferingSettings commands_buffering_settings);
  void SetReplicationMonitoringSettings(
      const ReplicationMonitoringSettings& replication_monitoring_settings);
  void SetClusterAutoTopology(bool auto_topology);

  // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
  boost::signals2::signal<void(size_t shard)> signal_instances_changed;
  // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
  boost::signals2::signal<void()> signal_not_in_cluster_mode;

  Request MakeRequest(CmdArgs&& args, const std::string& key,
                      bool master = true,
                      const CommandControl& command_control = {},
                      size_t replies_to_skip = 0) {
    return {*this,
            std::forward<CmdArgs>(args),
            key,
            master,
            command_control,
            replies_to_skip};
  }

  Request MakeRequest(CmdArgs&& args, size_t shard, bool master = true,
                      const CommandControl& command_control = {},
                      size_t replies_to_skip = 0) {
    return {*this,           std::forward<CmdArgs>(args),
            shard,           master,
            command_control, replies_to_skip};
  }

  std::vector<Request> MakeRequests(CmdArgs&& args, bool master = true,
                                    const CommandControl& command_control = {},
                                    size_t replies_to_skip = 0);

  CommandControl GetCommandControl(const CommandControl& cc) const;

  virtual void SetConfigDefaultCommandControl(
      const std::shared_ptr<CommandControl>& cc);

  using UserMessageCallback = std::function<void(const std::string& channel,
                                                 const std::string& message)>;
  using UserPmessageCallback =
      std::function<void(const std::string& pattern, const std::string& channel,
                         const std::string& message)>;

  using MessageCallback =
      std::function<void(ServerId server_id, const std::string& channel,
                         const std::string& message)>;
  using PmessageCallback = std::function<void(
      ServerId server_id, const std::string& pattern,
      const std::string& channel, const std::string& message)>;
  using SubscribeCallback =
      std::function<void(ServerId, const std::string& channel, size_t count)>;
  using UnsubscribeCallback =
      std::function<void(ServerId, const std::string& channel, size_t count)>;

 protected:
  std::vector<std::shared_ptr<const Shard>> GetMasterShards() const;

  // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
  std::unique_ptr<SentinelImplBase> impl_;

 public:
  static void OnSubscribeReply(MessageCallback message_callback,
                               SubscribeCallback subscribe_callback,
                               UnsubscribeCallback unsubscribe_callback,
                               ReplyPtr reply);

  static void OnPsubscribeReply(PmessageCallback pmessage_callback,
                                SubscribeCallback subscribe_callback,
                                UnsubscribeCallback unsubscribe_callback,
                                ReplyPtr reply);

 private:
  size_t GetPublishShard(PubShard policy);

  void CheckRenameParams(const std::string& key,
                         const std::string& newkey) const;

  friend class Transaction;

  std::shared_ptr<ThreadPools> thread_pools_;
  std::unique_ptr<engine::ev::ThreadControl> sentinel_thread_control_;
  CommandControl secdist_default_command_control_;
  utils::SwappingSmart<CommandControl> config_default_command_control_;
  std::atomic_int publish_shard_{0};
  testsuite::RedisControl testsuite_redis_control_;
};

}  // namespace redis

USERVER_NAMESPACE_END
