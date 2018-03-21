#pragma once

#include <functional>
#include <map>
#include <mutex>
#include <set>
#include <shared_mutex>
#include <sstream>
#include <string>
#include <vector>

#include <engine/ev/thread_pool.hpp>

#include "base.hpp"
#include "command.hpp"
#include "key_shard.hpp"
#include "redis.hpp"

namespace storages {
namespace redis {

class SentinelImpl : public engine::ev::ThreadControl {
 public:
  using ReadyChangeCallback = std::function<void(
      size_t shard, const std::string& shard_name, bool master, bool ready)>;

  SentinelImpl(const engine::ev::ThreadControl& thread_control,
               engine::ev::ThreadPool& thread_pool,
               const std::vector<std::string>& shards,
               const std::vector<ConnectionInfo>& conns, std::string password,
               ReadyChangeCallback ready_callback,
               std::unique_ptr<KeyShard>&& key_shard, bool track_masters,
               bool track_slaves);
  ~SentinelImpl();

  static constexpr size_t unknown_shard = -1;

  struct SentinelCommand {
    CommandPtr command;
    bool master = true;
    size_t shard = unknown_shard;
    std::chrono::steady_clock::time_point start;

    SentinelCommand() {}
    SentinelCommand(CommandPtr command, bool master, size_t shard,
                    std::chrono::steady_clock::time_point start)
        : command(command), master(master), shard(shard), start(start) {}
  };

  static void InvokeCommand(CommandPtr command, ReplyPtr reply);
  void AsyncCommand(const SentinelCommand& scommand);

  size_t ShardByKey(const std::string& key) const;

  size_t ShardCount() const { return master_shards_.size(); }

  Stat GetStat();

 private:
  static constexpr const std::chrono::milliseconds cluster_slots_timeout_{4000};

  class ConnectionInfoInt : public ConnectionInfo {
   public:
    std::string name;

    bool operator==(const ConnectionInfoInt& o) const {
      return Fulltext() == o.Fulltext();
    }
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

  struct ConnectionStatus {
    ConnectionInfoInt info;
    std::shared_ptr<Redis> instance;
  };

  class SlotInfo {
   public:
    struct ShardInterval {
      size_t slot_min;
      size_t slot_max;
      size_t shard;

      ShardInterval() {}
      ShardInterval(size_t slot_min, size_t slot_max, size_t shard)
          : slot_min(slot_min), slot_max(slot_max), shard(shard) {}
    };

    SlotInfo();

    size_t ShardBySlot(size_t slot) const;
    void UpdateSlots(const std::vector<ShardInterval>& intervals);

   private:
    struct SlotShard {
      size_t bound;
      size_t shard;

      SlotShard() {}
      SlotShard(size_t bound, size_t shard) : bound(bound), shard(shard) {}
    };

    mutable std::mutex mutex_;
    std::vector<SlotShard> slot_shards_;
  };

  class ShardInfo {
   public:
    using HostPortToShardMap = std::map<std::pair<std::string, size_t>, size_t>;

    size_t GetShard(const std::string& host, int port) const;
    void UpdateHostPortToShard(HostPortToShardMap&& host_port_to_shard_new);

   private:
    mutable std::mutex mutex_;
    HostPortToShardMap host_port_to_shard;
  };

  using ConnInfoByShard = std::vector<ConnectionInfoInt>;
  using ConnInfoMap = std::map<std::string, std::vector<ConnectionInfoInt>>;

  struct Shard {
    std::string name;
    std::shared_timed_mutex mutex;
    std::set<ConnectionInfoInt> info;
    std::vector<ConnectionStatus> instances;
    std::vector<ConnectionStatus> clean_wait;
    size_t current = 0;
    std::function<void(Redis::State state)> state_change_callback;
    std::function<void(bool ready)> ready_change_callback;
    bool prev_connected = false;
    bool read_only = false;
    bool destroying = false;

    bool AsyncCommand(CommandPtr command, size_t* pinstance_idx = nullptr);

    void Clean();

    void ProcessCreation(
        const std::function<engine::ev::ThreadControl&()>& get_thread);
    void ProcessStateUpdate();
    bool SetConnectionInfo(const std::vector<ConnectionInfoInt>& info_array,
                           std::function<void(Redis::State state)> callback);
    std::vector<Stat> GetStat();
  };

  void Start();
  void Stop();

  static void OnCheckTimer(struct ev_loop* loop, ev_timer* w, int revents);
  static void ChangedState(struct ev_loop* loop, ev_async* w, int revents);
  static void UpdateInstances(struct ev_loop* loop, ev_async* w, int revents);
  static void OnModifyConnectionInfo(struct ev_loop* loop, ev_async* w,
                                     int revents);
  static void CommandLoop(struct ev_loop* loop, ev_async* w, int revents);

  void OnCheckTimerImpl();
  void ReadSentinels();
  void CheckConnections();
  void UpdateInstancesImpl();
  void CommandLoopImpl();

  void SentinelGetHosts(
      CmdArgs&& command, bool allow_empty,
      std::function<void(const ConnInfoByShard& info)>&& callback);
  ConnInfoMap ConvertConnectionInfoVectorToMap(
      const std::vector<ConnectionInfoInt>& array);
  bool SetConnectionInfo(const ConnInfoMap& info_by_shards,
                         std::vector<std::shared_ptr<Shard>>& shards);

  void EnqueueCommand(const SentinelCommand& scommand);

  size_t ParseMovedShard(const std::string& err_string);

  void UpdateClusterSlots(size_t shard);

 private:
  static size_t HashSlot(const std::string& key);

  engine::ev::ThreadPool& thread_pool_;
  struct ev_loop* loop_ = nullptr;
  ev_async watch_state_;
  ev_async watch_update_;
  ev_async watch_create_;
  ev_async watch_command_;
  ev_timer check_timer_;
  std::mutex sentinels_mutex_;
  std::vector<std::shared_ptr<Shard>> master_shards_;
  std::vector<std::shared_ptr<Shard>> slaves_shards_;
  ConnInfoByShard master_shards_info_;
  ConnInfoByShard slaves_shards_info_;
  Shard sentinels_;
  std::map<std::string, size_t> shards_;
  ShardInfo shard_info_;
  std::string password_;
  bool exit_request_ = false;
  double check_interval_ = 3.0;
  ReadyChangeCallback ready_callback_;
  bool track_masters_;
  bool track_slaves_;
  SlotInfo slot_info_;

  std::mutex command_mutex_;
  std::vector<SentinelCommand> commands_;
  bool destroying_ = false;

  size_t current_slots_shard_ = 0;
  std::unique_ptr<KeyShard> key_shard_ = nullptr;
};

}  // namespace redis
}  // namespace storages
