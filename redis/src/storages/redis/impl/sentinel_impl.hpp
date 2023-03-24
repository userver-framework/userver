#pragma once

#include <array>
#include <atomic>
#include <chrono>
#include <map>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include <boost/signals2/signal.hpp>

#include <engine/ev/thread_control.hpp>
#include <engine/ev/thread_pool.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/impl/condition_variable_any.hpp>
#include <userver/utils/swappingsmart.hpp>

#include <storages/redis/impl/redis_stats.hpp>
#include <userver/storages/redis/impl/types.hpp>
#include <userver/storages/redis/impl/wait_connected_mode.hpp>

#include "ev_wrapper.hpp"
#include "keys_for_shards.hpp"
#include "keyshard_impl.hpp"
#include "redis.hpp"
#include "sentinel_query.hpp"
#include "shard.hpp"

USERVER_NAMESPACE_BEGIN

namespace redis {

class SentinelImpl {
 public:
  using ReadyChangeCallback = std::function<void(
      size_t shard, const std::string& shard_name, bool ready)>;

  SentinelImpl(const engine::ev::ThreadControl& sentinel_thread_control,
               const std::shared_ptr<engine::ev::ThreadPool>& redis_thread_pool,
               Sentinel& sentinel, const std::vector<std::string>& shards,
               const std::vector<ConnectionInfo>& conns,
               std::string shard_group_name, const std::string& client_name,
               const Password& password, ConnectionSecurity connection_security,
               ReadyChangeCallback ready_callback,
               std::unique_ptr<KeyShard>&& key_shard,
               ConnectionMode mode = ConnectionMode::kCommands);
  ~SentinelImpl();

  std::unordered_map<ServerId, size_t, ServerIdHasher>
  GetAvailableServersWeighted(size_t shard_idx, bool with_master,
                              const CommandControl& cc = {}) const;

  void WaitConnectedDebug(bool allow_empty_slaves);

  void WaitConnectedOnce(RedisWaitConnected wait_connected);

  void ForceUpdateHosts();

  static constexpr size_t kUnknownShard =
      std::numeric_limits<std::size_t>::max();

  struct SentinelCommand {
    CommandPtr command;
    bool master = true;
    size_t shard = kUnknownShard;
    std::chrono::steady_clock::time_point start;

    SentinelCommand() = default;
    SentinelCommand(CommandPtr command, bool master, size_t shard,
                    std::chrono::steady_clock::time_point start)
        : command(command), master(master), shard(shard), start(start) {}
  };

  void AsyncCommand(const SentinelCommand& scommand,
                    size_t prev_instance_idx = -1);
  void AsyncCommandToSentinel(CommandPtr command);
  size_t ShardByKey(const std::string& key) const;
  size_t ShardsCount() const { return master_shards_.size(); }
  const std::string& GetAnyKeyForShard(size_t shard_idx) const;
  SentinelStatistics GetStatistics(const MetricsSettings& settings) const;

  void Init();
  void Start();
  void Stop();

  std::vector<std::shared_ptr<const Shard>> GetMasterShards() const;
  bool IsInClusterMode() const;

  void SetCommandsBufferingSettings(
      CommandsBufferingSettings commands_buffering_settings);
  void SetReplicationMonitoringSettings(
      const ReplicationMonitoringSettings& replication_monitoring_settings);

 private:
  static constexpr const std::chrono::milliseconds cluster_slots_timeout_ =
      std::chrono::milliseconds(4000);

  class SlotInfo {
   public:
    struct ShardInterval {
      size_t slot_min;
      size_t slot_max;
      size_t shard;

      ShardInterval(size_t slot_min, size_t slot_max, size_t shard)
          : slot_min(slot_min), slot_max(slot_max), shard(shard) {}
    };

    SlotInfo();

    size_t ShardBySlot(size_t slot) const;
    void UpdateSlots(const std::vector<ShardInterval>& intervals);
    bool IsInitialized() const;
    bool WaitInitialized(engine::Deadline deadline);

   private:
    std::mutex mutex_;
    engine::impl::ConditionVariableAny<std::mutex> cv_;
    std::atomic<bool> is_initialized_{false};

    std::array<std::atomic<size_t>, kClusterHashSlots> slot_to_shard_{};
  };

  class ShardInfo {
   public:
    using HostPortToShardMap = std::map<std::pair<std::string, size_t>, size_t>;

    size_t GetShard(const std::string& host, int port) const;
    void UpdateHostPortToShard(HostPortToShardMap&& host_port_to_shard_new);

   private:
    HostPortToShardMap host_port_to_shard_;
    mutable std::mutex mutex_;
  };

  class ConnectedStatus {
   public:
    void SetMasterReady();
    void SetSlaveReady();

    bool WaitReady(engine::Deadline deadline, WaitConnectedMode mode);

   private:
    template <typename Pred>
    bool Wait(engine::Deadline deadline, const Pred& pred);

    std::mutex mutex_;
    std::atomic<bool> master_ready_{false};
    std::atomic<bool> slave_ready_{false};

    engine::impl::ConditionVariableAny<std::mutex> cv_;
  };

  void InitKeyShard();

  void GenerateKeysForShards(size_t max_len = 4);
  void AsyncCommandFailed(const SentinelCommand& scommand);

  static void OnCheckTimer(struct ev_loop*, ev_timer* w, int revents) noexcept;
  static void ChangedState(struct ev_loop*, ev_async* w, int revents) noexcept;
  static void UpdateInstances(struct ev_loop*, ev_async* w,
                              int revents) noexcept;
  static void OnModifyConnectionInfo(struct ev_loop*, ev_async* w,
                                     int revents) noexcept;
  static void OnUpdateClusterSlotsRequested(struct ev_loop*, ev_async* w,
                                            int revents) noexcept;

  void ProcessCreationOfShards(std::vector<std::shared_ptr<Shard>>& shards);

  void RefreshConnectionInfo();
  void ReadSentinels();
  void ReadClusterHosts();
  void CheckConnections();
  void UpdateInstancesImpl();
  bool SetConnectionInfo(ConnInfoMap info_by_shards,
                         std::vector<std::shared_ptr<Shard>>& shards);
  void EnqueueCommand(const SentinelCommand& command);
  size_t ParseMovedShard(const std::string& err_string);
  void RequestUpdateClusterSlots(size_t shard);
  void UpdateClusterSlots(size_t shard);
  void DoUpdateClusterSlots(ReplyPtr reply);
  void InitShards(const std::vector<std::string>& shards,
                  std::vector<std::shared_ptr<Shard>>& shard_objects,
                  const ReadyChangeCallback& ready_callback);

  static size_t HashSlot(const std::string& key);

  void ProcessWaitingCommands();

  Sentinel& sentinel_obj_;
  engine::ev::ThreadControl ev_thread_;

  std::string shard_group_name_;
  std::shared_ptr<const std::vector<std::string>> init_shards_;
  std::vector<std::unique_ptr<ConnectedStatus>> connected_statuses_;
  std::vector<ConnectionInfo> conns_;
  ReadyChangeCallback ready_callback_;

  std::shared_ptr<engine::ev::ThreadPool> redis_thread_pool_;
  ev_async watch_state_{};
  ev_async watch_update_{};
  ev_async watch_create_{};
  ev_async watch_cluster_slots_{};
  ev_timer check_timer_{};
  mutable std::mutex sentinels_mutex_;
  std::vector<std::shared_ptr<Shard>> master_shards_;  // TODO rename
  ConnInfoByShard master_shards_info_;
  ConnInfoByShard slaves_shards_info_;
  std::shared_ptr<Shard> sentinels_;
  std::map<std::string, size_t> shards_;
  ShardInfo shard_info_;
  std::string client_name_;
  Password password_{std::string()};
  ConnectionSecurity connection_security_;
  double check_interval_;
  std::atomic<bool> update_cluster_slots_flag_;
  std::atomic<bool> cluster_mode_failed_;  // also false if not in cluster mode
  std::vector<SentinelCommand> commands_;
  std::mutex command_mutex_;
  std::atomic<size_t> current_slots_shard_ = 0;
  utils::SwappingSmart<KeyShard> key_shard_;
  ConnectionMode connection_mode_;
  std::unique_ptr<SlotInfo> slot_info_;
  SentinelStatisticsInternal statistics_internal_;
  utils::SwappingSmart<KeysForShards> keys_for_shards_;
  std::optional<CommandsBufferingSettings> commands_buffering_settings_;
};

}  // namespace redis

USERVER_NAMESPACE_END
