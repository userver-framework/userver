#pragma once

#include <set>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <userver/utils/swappingsmart.hpp>

#include <storages/redis/impl/redis.hpp>
#include <storages/redis/impl/redis_stats.hpp>

USERVER_NAMESPACE_BEGIN

namespace redis {

class ConnectionInfoInt {
 public:
  ConnectionInfoInt() = default;
  explicit ConnectionInfoInt(ConnectionInfo conn_info);

  const std::string& Name() const;
  void SetName(std::string);

  std::pair<std::string, int> HostPort() const;

  void SetPassword(Password);

  bool IsReadOnly() const;
  void SetReadOnly(bool);

  void SetConnectionSecurity(ConnectionSecurity value);
  ConnectionSecurity GetConnectionSecurity() const;

  const std::string& Fulltext() const;

  void Connect(Redis&) const;

 private:
  ConnectionInfo conn_info_;
  std::string name_;
  std::string fulltext_;
};

bool operator==(const ConnectionInfoInt&, const ConnectionInfoInt&);
bool operator!=(const ConnectionInfoInt&, const ConnectionInfoInt&);
bool operator<(const ConnectionInfoInt&, const ConnectionInfoInt&);

using ConnInfoMap = std::map<std::string, std::vector<ConnectionInfoInt>>;

struct ConnectionStatus {
  ConnectionInfoInt info;
  std::shared_ptr<Redis> instance;
};

using ConnInfoByShard = std::vector<ConnectionInfoInt>;

class Shard {
 public:
  struct Options {
    std::string shard_name;
    std::string shard_group_name;
    bool cluster_mode{false};
    std::function<void(bool ready)> ready_change_callback;
    std::vector<ConnectionInfo> connection_infos;
  };

  explicit Shard(Options options);

  std::unordered_map<ServerId, size_t, ServerIdHasher>
  GetAvailableServersWeighted(bool with_master,
                              const CommandControl& command_control = {}) const;

  std::vector<ServerId> GetAllInstancesServerId() const;

  bool AsyncCommand(CommandPtr command);
  std::shared_ptr<Redis> GetInstance(
      const std::vector<unsigned char>& available_servers,
      bool may_fallback_to_any, size_t skip_idx, bool read_only,
      size_t* pinstance_idx);
  void Clean();
  bool ProcessCreation(
      const std::shared_ptr<engine::ev::ThreadPool>& redis_thread_pool);
  bool ProcessStateUpdate();
  bool SetConnectionInfo(std::vector<ConnectionInfoInt> info_array);
  bool IsConnectedToAllServersDebug(bool allow_empty) const;
  ShardStatistics GetStatistics(bool master,
                                const MetricsSettings& settings) const;
  size_t InstancesSize() const;
  const std::string& ShardName() const;
  boost::signals2::signal<void(ServerId, Redis::State)>&
  SignalInstanceStateChange();
  boost::signals2::signal<void()>& SignalNotInClusterMode();
  boost::signals2::signal<void(ServerId, bool)>& SignalInstanceReady();

  void SetCommandsBufferingSettings(
      CommandsBufferingSettings commands_buffering_settings);
  void SetReplicationMonitoringSettings(
      const ReplicationMonitoringSettings& replication_monitoring_settings);

 private:
  std::vector<unsigned char> GetAvailableServers(
      const CommandControl& command_control, bool with_masters,
      bool with_slaves) const;
  std::vector<unsigned char> GetNearestServersPing(
      const CommandControl& command_control, bool with_masters,
      bool with_slaves) const;

  std::vector<ConnectionInfoInt> GetConnectionInfosToCreate() const;
  bool UpdateCleanWaitQueue(std::vector<ConnectionStatus>&& add_clean_wait);

  const std::string shard_name_;
  const std::string shard_group_name_;
  std::atomic_size_t current_{0};

  mutable std::shared_mutex mutex_;
  std::vector<ConnectionInfoInt> connection_infos_;
  std::vector<ConnectionStatus> instances_;
  std::vector<ConnectionStatus> clean_wait_;
  std::chrono::steady_clock::time_point last_connected_time_;
  std::chrono::steady_clock::time_point last_ready_time_ =
      std::chrono::steady_clock::now();
  bool destroying_ = false;

  const std::function<void(bool ready)> ready_change_callback_;

  boost::signals2::signal<void(ServerId, Redis::State)>
      signal_instance_state_change_;
  boost::signals2::signal<void()> signal_not_in_cluster_mode_;
  boost::signals2::signal<void(ServerId, bool)> signal_instance_ready_;

  utils::SwappingSmart<CommandsBufferingSettings> commands_buffering_settings_;

  bool prev_connected_ = false;
  const bool cluster_mode_ = false;
};

}  // namespace redis

USERVER_NAMESPACE_END
