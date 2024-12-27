#ifndef IMPL_STANDALONE_IMPL_HPP
#define IMPL_STANDALONE_IMPL_HPP
#include "sentinel_impl.hpp"

#include <storages/redis/impl/redis_connection_holder.hpp>
#include <storages/redis/impl/cluster_shard.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::ev {
class PeriodicWatcher;
}

namespace storages::redis::impl {


class StandaloneImpl : public SentinelImplBase {
 public:
  using ReadyChangeCallback = std::function<void(
      size_t shard, const std::string& shard_name, bool ready)>;
  using SentinelCommand = SentinelImplBase::SentinelCommand;

  static constexpr std::size_t kUnknownShard =
      std::numeric_limits<std::size_t>::max();
  StandaloneImpl(
      const engine::ev::ThreadControl& sentinel_thread_control,
      const std::shared_ptr<engine::ev::ThreadPool>& redis_thread_pool,
      ConnectionInfo conn, std::string shard_group_name,
      const std::string& client_name, const Password& password,
      ConnectionSecurity connection_security,
      ReadyChangeCallback ready_callback,
      dynamic_config::Source dynamic_config_source,
      ConnectionMode mode = ConnectionMode::kCommands);
  ~StandaloneImpl() override;

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
  void SetRetryBudgetSettings(
      const utils::RetryBudgetSettings& settings) override;
  PublishSettings GetPublishSettings() override;

  static size_t GetClusterSlotsCalledCounter();

 private:
  void AsyncCommandFailed(const SentinelCommand& scommand);
  void EnqueueCommand(const SentinelCommand& command);

  engine::ev::ThreadControl ev_thread_;

  std::unique_ptr<engine::ev::PeriodicWatcher> process_waiting_commands_timer_;
  void ProcessWaitingCommands();
  void ProcessWaitingCommandsOnStop();

  std::string shard_group_name_;
  ConnectionInfo conn_;
  ReadyChangeCallback ready_callback_;

  std::shared_ptr<engine::ev::ThreadPool> redis_thread_pool_;

  std::string client_name_;
  Password password_{std::string()};

  std::vector<SentinelCommand> commands_;
  std::mutex command_mutex_;

  SentinelStatisticsInternal statistics_internal_;

  dynamic_config::Source dynamic_config_source_;

  std::shared_ptr<RedisConnectionHolder> connection_holder_;
  ClusterShard master_shard_;
};

}  // namespace storages::redis::impl

USERVER_NAMESPACE_END


#endif    /* IMPL_STANDALONE_IMPL_HPP */
