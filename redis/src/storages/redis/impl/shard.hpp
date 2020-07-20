#pragma once

#include <boost/thread/shared_mutex.hpp>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include <storages/redis/impl/redis_stats.hpp>
#include "redis.hpp"

namespace redis {

class ConnectionInfoInt : public ConnectionInfo {
 public:
  std::string name;

  bool operator==(const ConnectionInfoInt& o) const {
    return Fulltext() == o.Fulltext();
  }
  bool operator!=(const ConnectionInfoInt& o) const { return !(*this == o); }
  bool operator<(const ConnectionInfoInt& o) const {
    return Fulltext() < o.Fulltext();
  }

  inline const std::string& Fulltext() const {
    if (fulltext_.empty()) fulltext_ = host + ":" + std::to_string(port);
    return fulltext_;
  }

 private:
  mutable std::string fulltext_;
};

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
    bool cluster_mode;
    bool read_only;
    std::function<void(bool ready)> ready_change_callback;
    std::vector<ConnectionInfo> connection_infos;
  };

  explicit Shard(Options options);

  std::unordered_map<ServerId, size_t, ServerIdHasher>
  GetAvailableServersWeighted(const CommandControl& command_control = {}) const;

  std::vector<unsigned char> GetAvailableServers(
      const CommandControl& command_control) const;
  std::vector<unsigned char> GetNearestServerConductor(
      const CommandControl& command_control) const;
  std::vector<unsigned char> GetNearestServersPing(
      const CommandControl& command_control) const;

  std::vector<ServerId> GetAllInstancesServerId() const;

  bool AsyncCommand(CommandPtr command, size_t* pinstance_idx = nullptr);
  std::shared_ptr<Redis> GetInstance(
      const std::vector<unsigned char>& available_servers,
      bool may_fallback_to_any, size_t skip_idx, size_t* pinstance_idx);
  void Clean();
  bool ProcessCreation(
      const std::shared_ptr<engine::ev::ThreadPool>& redis_thread_pool);
  bool ProcessStateUpdate();
  bool SetConnectionInfo(const std::vector<ConnectionInfoInt>& info_array);
  bool IsConnectedToAllServersDebug(bool allow_empty);
  ShardStatistics GetStatistics() const;
  size_t InstancesSize() const;
  const std::string& ShardName() const;
  boost::signals2::signal<void(ServerId, Redis::State)>&
  SignalInstanceStateChange();
  boost::signals2::signal<void()>& SignalNotInClusterMode();
  boost::signals2::signal<void(ServerId)>& SignalInstanceReady();

 private:
  std::set<ConnectionInfoInt> GetConnectionInfosToCreate() const;
  bool UpdateCleanWaitQueue(std::vector<ConnectionStatus>&& add_clean_wait);

  const std::string shard_name_;
  const std::string shard_group_name_;
  std::atomic_size_t current_{0};

  mutable boost::shared_mutex mutex_;
  std::set<ConnectionInfoInt> connection_infos_;
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
  boost::signals2::signal<void(ServerId)> signal_instance_ready_;

  bool prev_connected_ = false;
  const bool cluster_mode_ = false;
  const bool read_only_ = false;
};

}  // namespace redis
