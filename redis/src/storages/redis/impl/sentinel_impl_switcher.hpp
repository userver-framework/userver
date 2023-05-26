#pragma once
#include "sentinel_impl.hpp"
#include "userver/engine/task/task_with_result.hpp"
#include "userver/utils/periodic_task.hpp"

#include <userver/utils/swappingsmart.hpp>

USERVER_NAMESPACE_BEGIN

namespace redis {

/// Class stores actual implementation of Sentinel and allow to switch it in
/// runtime. If settings changed then new implementation created in separate
/// coroutine. New implementation become visible after it connected to database,
/// so switch must not lead to loses.
/// Must be removed and replaced with ClusterSentinelImpl after TAXICOMMON-6018
class ClusterSentinelImplSwitcher : public SentinelImplBase {
 public:
  using ReadyChangeCallback = std::function<void(
      size_t shard, const std::string& shard_name, bool ready)>;
  using SentinelCommand = SentinelImplBase::SentinelCommand;

  static constexpr size_t kUnknownShard =
      std::numeric_limits<std::size_t>::max();

  ClusterSentinelImplSwitcher(
      engine::ev::ThreadControl& sentinel_thread_control,
      const std::shared_ptr<engine::ev::ThreadPool>& redis_thread_pool,
      Sentinel& sentinel, const std::vector<std::string>& shards,
      const std::vector<ConnectionInfo>& conns, std::string shard_group_name,
      const std::string& client_name, const Password& password,
      ConnectionSecurity connection_security,
      ReadyChangeCallback ready_callback, std::unique_ptr<KeyShard>&& key_shard,
      dynamic_config::Source dynamic_config_source,
      ConnectionMode mode = ConnectionMode::kCommands);
  ~ClusterSentinelImplSwitcher() override;

  /// SentinelImplBase interface
  /// @{
  std::unordered_map<ServerId, size_t, ServerIdHasher>
  GetAvailableServersWeighted(size_t shard_idx, bool with_master,
                              const CommandControl& cc /*= {}*/) const override;

  void WaitConnectedDebug(bool allow_empty_slaves) override;
  void WaitConnectedOnce(RedisWaitConnected wait_connected) override;
  void ForceUpdateHosts() override;
  void AsyncCommand(const SentinelCommand& scommand,
                    size_t prev_instance_idx /*= -1*/) override;
  void AsyncCommandToSentinel(CommandPtr command) override;
  size_t ShardByKey(const std::string& key) const override;

  size_t ShardsCount() const override;
  const std::string& GetAnyKeyForShard(size_t shard_idx) const override;
  SentinelStatistics GetStatistics(
      const MetricsSettings& settings) const override;

  void Init() override;
  void Start() override;
  void Stop() override;

  std::vector<std::shared_ptr<const Shard>> GetMasterShards() const override;
  bool IsInClusterMode() const override;
  void SetCommandsBufferingSettings(
      CommandsBufferingSettings commands_buffering_settings) override;
  void SetReplicationMonitoringSettings(
      const ReplicationMonitoringSettings& replication_monitoring_settings)
      override;
  void SetClusterAutoTopology(bool auto_topology) override;
  ///@}

  void SetEnabledByConfig(bool auto_topology);
  void UpdateImpl(bool async, bool wait);

 private:
  bool IsAutoTopologySentinel() const;
  bool IsUniversalSentinel() const;

  struct Params {
    engine::ev::ThreadControl& sentinel_thread_control;
    const std::shared_ptr<engine::ev::ThreadPool>& redis_thread_pool;
    Sentinel& sentinel;
    std::vector<std::string> shards;
    std::vector<ConnectionInfo> conns;
    std::string shard_group_name;
    std::string client_name;
    Password password;
    ConnectionSecurity connection_security;
    ReadyChangeCallback ready_callback;
    /// Always nullptr in cluster mode
    std::unique_ptr<KeyShard> key_shard;
    dynamic_config::Source dynamic_config_source;
    ConnectionMode mode;
  };
  Params params_;

  USERVER_NAMESPACE::redis::RedisWaitConnected wait_connected_;
  utils::SwappingSmart<SentinelImplBase> impl_;

  engine::Task create_task_;
  std::atomic<bool> enabled_by_config_ = false;
  std::atomic<bool> creating_impl_ = false;
};

}  // namespace redis

USERVER_NAMESPACE_END
