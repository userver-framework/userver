#include "cluster_sentinel_impl.hpp"

#include <fmt/format.h>
#include <atomic>
#include <boost/crc.hpp>

#include <userver/concurrent/variable.hpp>
#include <userver/rcu/rcu.hpp>
#include <userver/storages/redis/impl/exception.hpp>
#include <userver/storages/redis/impl/reply.hpp>
#include <userver/utils/fast_scope_guard.hpp>
#include <userver/utils/text.hpp>

#include <engine/ev/watcher.hpp>
#include <engine/ev/watcher/async_watcher.hpp>
#include <engine/ev/watcher/periodic_watcher.hpp>
#include <storages/redis/impl/cluster_topology.hpp>
#include <storages/redis/impl/sentinel.hpp>

USERVER_NAMESPACE_BEGIN

namespace redis {

namespace {
const auto kProcessCreationInterval = std::chrono::seconds(3);

struct StdMutexRcuTraits {
  using MutexType = std::mutex;
};

bool CheckQuorum(size_t requests_sent, size_t responses_parsed) {
  const size_t quorum = requests_sent / 2 + 1;
  return responses_parsed >= quorum;
}

size_t HashSlot(const std::string& key) {
  size_t start = 0;
  size_t len = 0;
  GetRedisKey(key, &start, &len);
  return std::for_each(key.data() + start, key.data() + start + len,
                       boost::crc_optimal<16, 0x1021>())() &
         0x3fff;
}

std::string ParseMovedShard(const std::string& err_string) {
  static const auto kUnknownShard = std::string("");
  size_t pos = err_string.find(' ');  // skip "MOVED" or "ASK"
  if (pos == std::string::npos) return kUnknownShard;
  pos = err_string.find(' ', pos + 1);  // skip hash_slot
  if (pos == std::string::npos) return kUnknownShard;
  pos++;
  size_t end = err_string.find(' ', pos);
  if (end == std::string::npos) end = err_string.size();
  const size_t colon_pos = err_string.rfind(':', end);
  int port = 0;
  try {
    port = std::stoi(err_string.substr(colon_pos + 1, end - (colon_pos + 1)));
  } catch (const std::exception& ex) {
    LOG_WARNING() << "exception in " << __func__ << "(\"" << err_string
                  << "\") " << ex.what();
    return kUnknownShard;
  }
  return err_string.substr(pos, colon_pos - pos) + ":" + std::to_string(port);
}

struct CommandSpecialPrinter {
  const CommandPtr& command;
};

logging::LogHelper& operator<<(logging::LogHelper& os,
                               CommandSpecialPrinter v) {
  const auto& command = v.command;

  if (command->args.args.size() == 1 ||
      command->invoke_counter + 1 >= command->args.args.size()) {
    os << command->args;
  } else if (command->invoke_counter < command->args.args.size() &&
             !command->args.args[command->invoke_counter].empty()) {
    os << fmt::format("subrequest idx={}, cmd={}", command->invoke_counter,
                      command->args.args[command->invoke_counter].front());
  }

  return os;
}

std::shared_ptr<const std::vector<std::string>> MakeShardNames() {
  /// From suggested max count of nodes ~1000, with replicas, so got ~500 shards
  static const size_t kMaxClusterShards = 500;
  std::vector<std::string> shard_names;
  shard_names.reserve(kMaxClusterShards);
  for (size_t i = 0; i < kMaxClusterShards; ++i) {
    auto number = std::to_string(i);
    if (number.size() < 2) {
      number.insert(0, "0");
    }
    auto name = "shard" + number;
    shard_names.push_back(std::move(name));
  }
  return std::make_shared<const std::vector<std::string>>(
      std::move(shard_names));
}

inline void InvokeCommand(CommandPtr command, ReplyPtr&& reply) {
  UASSERT(reply);
  if (reply->server_id.IsAny())
    reply->server_id = command->control.force_server_id;
  LOG_DEBUG() << "redis_request( " << CommandSpecialPrinter{command}
              << " ):" << (reply->status == ReplyStatus::kOk ? '+' : '-') << ":"
              << reply->time * 1000.0 << " cc: " << command->control.ToString()
              << command->log_extra;
  ++command->invoke_counter;
  try {
    command->callback(command, reply);
  } catch (const std::exception& ex) {
    LOG_WARNING() << "exception in command->callback, cmd=" << reply->cmd << " "
                  << ex << command->log_extra;
  } catch (...) {
    LOG_WARNING() << "exception in command->callback, cmd=" << reply->cmd
                  << command->log_extra;
  }
}

}  // namespace

class ClusterTopologyHolder {
 public:
  ClusterTopologyHolder(
      const engine::ev::ThreadControl& sentinel_thread_control,
      const std::shared_ptr<engine::ev::ThreadPool>& redis_thread_pool,
      std::string shard_group_name, Password password,
      const std::vector<std::string>& /*shards*/,
      const std::vector<ConnectionInfo>& conns)
      : ev_thread_(sentinel_thread_control),
        redis_thread_pool_(redis_thread_pool),
        shard_group_name_(std::move(shard_group_name)),
        password_(std::move(password)),
        shards_names_(MakeShardNames()),
        conns_(conns),
        update_topology_timer_(
            ev_thread_, [this] { UpdateClusterTopology(); },
            kSentinelGetHostsCheckInterval),
        update_topology_watch_(ev_thread_, [this] { UpdateClusterTopology(); }),
        explore_nodes_timer_(
            ev_thread_, [this] { ExploreNodes(); },
            kSentinelGetHostsCheckInterval),
        sentinels_process_creation_(
            ev_thread_,
            [this] {
              if (sentinels_) {
                sentinels_->ProcessCreation(redis_thread_pool_);
                sentinels_->ProcessStateUpdate();
              }
            },
            kProcessCreationInterval),
        is_topology_received_(false),
        update_cluster_slots_flag_(false) {
    LOG_DEBUG() << "Created ClusterTopologyHolder, shard_group_name="
                << shard_group_name_;
  }

  void Init() {
    const constexpr bool kClusterMode = true;
    Shard::Options shard_options;
    shard_options.shard_name = "(sentinel)";
    shard_options.shard_group_name = shard_group_name_;
    shard_options.cluster_mode = kClusterMode;
    shard_options.connection_infos = conns_;
    shard_options.ready_change_callback = [this](bool ready) {
      if (ready) {
        this->sentinels_->ProcessCreation(redis_thread_pool_);
        SendUpdateClusterTopology();
      }
    };

    sentinels_ = std::make_shared<Shard>(std::move(shard_options));
    sentinels_->SignalInstanceStateChange().connect(
        [this](ServerId id, Redis::State state) {
          LOG_TRACE() << "Signaled server " << id.GetDescription()
                      << " state=" << Redis::StateToString(state);
          if (state != Redis::State::kInit) ProcessStateUpdate();
        });
    sentinels_->SignalInstanceReady().connect([](ServerId, bool /*ready*/) {});
    sentinels_->ProcessCreation(redis_thread_pool_);
  }

  void Start() {
    update_topology_watch_.Start();
    update_topology_timer_.Start();
    explore_nodes_timer_.Start();
    sentinels_process_creation_.Start();
  }

  void Stop() {
    ev_thread_.RunInEvLoopBlocking([this] {
      update_topology_timer_.Stop();
      explore_nodes_timer_.Stop();
      sentinels_process_creation_.Stop();
    });
    sentinels_->Clean();
    topology_.Cleanup();
    nodes_.Clear();
  }

  bool WaitReadyOnce(engine::Deadline deadline, WaitConnectedMode mode) {
    std::unique_lock<std::mutex> lock(mutex_);
    return cv_.WaitUntil(lock, deadline, [this, mode]() {
      if (!IsInitialized()) return false;
      auto ptr = topology_.Read();
      return ptr->IsReady(mode);
    });
  }

  rcu::ReadablePtr<ClusterTopology, StdMutexRcuTraits> GetTopology() const {
    return topology_.Read();
  }

  void SendUpdateClusterTopology() { update_topology_watch_.Send(); }

  std::shared_ptr<Redis> GetRedisInstance(const std::string& host_port) const {
    return std::const_pointer_cast<Redis>(nodes_.Get(host_port));
  }

  void GetStatistics(SentinelStatistics& stats,
                     const MetricsSettings& settings) const;

  void SetCommandsBufferingSettings(CommandsBufferingSettings settings) {
    {
      auto settings_ptr = commands_buffering_settings_.Lock();
      if (*settings_ptr == settings) {
        return;
      }
      *settings_ptr = settings;
    }
    for (const auto& node : nodes_) {
      node.second->SetCommandsBufferingSettings(settings);
    }
  }

  void SetReplicationMonitoringSettings(
      ReplicationMonitoringSettings settings) {
    {
      auto settings_ptr = monitoring_settings_.Lock();
      *settings_ptr = settings;
    }
    for (const auto& node : nodes_) {
      node.second->SetReplicationMonitoringSettings(settings);
    }
  }

  static size_t GetClusterSlotsCalledCounter() {
    return cluster_slots_call_counter_.load(std::memory_order_relaxed);
  }

 private:
  void ProcessStateUpdate() { sentinels_->ProcessStateUpdate(); }
  std::shared_ptr<Redis> CreateRedisInstance(const std::string& host_port);

  engine::ev::ThreadControl ev_thread_;
  std::shared_ptr<engine::ev::ThreadPool> redis_thread_pool_;

  std::string shard_group_name_;
  Password password_;
  std::shared_ptr<const std::vector<std::string>> shards_names_;
  std::vector<ConnectionInfo> conns_;
  std::shared_ptr<Shard> sentinels_;

  std::atomic_size_t current_topology_version_{0};
  rcu::Variable<ClusterTopology, StdMutexRcuTraits> topology_;

  /// Update cluster topology
  /// @{
  engine::ev::PeriodicWatcher update_topology_timer_;
  engine::ev::AsyncWatcher update_topology_watch_;
  void UpdateClusterTopology();
  /// @}

  engine::ev::PeriodicWatcher explore_nodes_timer_;
  void ExploreNodes();

  engine::ev::PeriodicWatcher sentinels_process_creation_;

  ///{ Wait ready
  std::mutex mutex_;
  engine::impl::ConditionVariableAny<std::mutex> cv_;
  std::atomic<bool> is_topology_received_{false};
  std::atomic<bool> is_nodes_received_{false};
  std::atomic<bool> update_cluster_slots_flag_{false};
  bool IsInitialized() const {
    return is_nodes_received_.load() && is_topology_received_.load();
  }
  ///}

  NodesStorage nodes_;

  concurrent::Variable<std::optional<CommandsBufferingSettings>, std::mutex>
      commands_buffering_settings_;
  concurrent::Variable<ReplicationMonitoringSettings, std::mutex>
      monitoring_settings_;

  static std::atomic<size_t> cluster_slots_call_counter_;
};

std::atomic<size_t> ClusterTopologyHolder::cluster_slots_call_counter_(0);

namespace {
enum class ClusterNodesResponseStatus {
  kOk,
  kFail,
  kNonCluster,
};
ClusterNodesResponseStatus ParseCLusterNodesResponse(
    const ReplyPtr& reply, std::unordered_set<std::string>& res) {
  UASSERT(reply);
  if (reply->IsUnknownCommandError()) {
    return ClusterNodesResponseStatus::kNonCluster;
  }

  if (!reply->IsOk()) {
    return ClusterNodesResponseStatus::kFail;
  }

  if (!reply->data.IsString()) {
    return ClusterNodesResponseStatus::kFail;
  }
  const auto& host_lines =
      utils::text::SplitIntoStringViewVector(reply->data.GetString(), "\n");

  for (const auto& host_line : host_lines) {
    const auto& splitted =
        utils::text::SplitIntoStringViewVector(host_line, " ");
    if (splitted.size() < 2) {
      continue;
    }

    const auto& host_port_communication_port = splitted[1];
    const auto host_port_it = host_port_communication_port.rfind('@');
    if (host_port_it == std::string::npos) {
      return ClusterNodesResponseStatus::kFail;
    }
    auto host_port = host_port_communication_port.substr(0, host_port_it);

    const auto port_it = host_port.rfind(':');
    if (port_it == std::string::npos) {
      return ClusterNodesResponseStatus::kFail;
    }
    res.emplace(std::move(host_port));
  }

  return ClusterNodesResponseStatus::kOk;
}

}  // namespace

void ClusterTopologyHolder::ExploreNodes() {
  /// CALL cluster nodes, parse, update moved_instances
  if (!sentinels_) {
    return;
  }

  const auto cmd = PrepareCommand(
      {"CLUSTER", "NODES"}, [this](const CommandPtr& /*cmd*/, ReplyPtr reply) {
        std::unordered_set<std::string> host_ports;

        if (ParseCLusterNodesResponse(reply, host_ports) !=
            ClusterNodesResponseStatus::kOk) {
          LOG_WARNING() << "Failed to parse CLUSTER NODES response";
          return;
        }

        bool got_new_node = false;
        for (auto&& host_port : host_ports) {
          if (nodes_.Get(host_port)) {
            /// already have this connection
            continue;
          }

          auto instance = CreateRedisInstance(host_port);
          nodes_.Insert(std::move(host_port), std::move(instance));
          got_new_node = true;
        }
        if (got_new_node) {
          LOG_DEBUG() << "Got new cluster nodes";
        }

        if (!is_nodes_received_.exchange(true)) {
          SendUpdateClusterTopology();
        }
      });
  sentinels_->AsyncCommand(cmd);
}

void ClusterSentinelImpl::ProcessWaitingCommands() {
  std::vector<SentinelCommand> waiting_commands;

  {
    const std::lock_guard<std::mutex> lock(command_mutex_);
    waiting_commands.swap(commands_);
  }
  if (!waiting_commands.empty()) {
    LOG_INFO() << "ProcessWaitingCommands client=" << client_name_
               << " shard_group_name=" << shard_group_name_
               << " waiting_commands.size()=" << waiting_commands.size();
  }

  const std::chrono::steady_clock::time_point now =
      std::chrono::steady_clock::now();
  for (const SentinelCommand& scommand : waiting_commands) {
    const auto& command = scommand.command;
    if (scommand.start + command->control.timeout_all < now) {
      for (const auto& args : command->args.args) {
        auto reply = std::make_shared<Reply>(
            args[0], nullptr, ReplyStatus::kTimeoutError,
            "Command in the send queue timed out");
        statistics_internal_.redis_not_ready++;
        InvokeCommand(command, std::move(reply));
      }
    } else {
      AsyncCommand(scommand, kDefaultPrevInstanceIdx);
    }
  }
}

void ClusterSentinelImpl::ProcessWaitingCommandsOnStop() {
  std::vector<SentinelCommand> waiting_commands;

  {
    const std::lock_guard<std::mutex> lock(command_mutex_);
    waiting_commands.swap(commands_);
  }

  for (const SentinelCommand& scommand : waiting_commands) {
    const auto& command = scommand.command;
    for (const auto& args : command->args.args) {
      auto reply = std::make_shared<Reply>(
          args[0], nullptr, ReplyStatus::kTimeoutError,
          "Stopping, killing commands remaining in send queue");
      statistics_internal_.redis_not_ready++;
      InvokeCommand(command, std::move(reply));
    }
  }
}

std::shared_ptr<Redis> ClusterTopologyHolder::CreateRedisInstance(
    const std::string& host_port) {
  const auto port_it = host_port.rfind(':');
  UINVARIANT(port_it != std::string::npos, "port must be delimited by ':'");
  const auto port_str = host_port.substr(port_it + 1);
  const auto port = std::stoi(port_str);
  const auto host = host_port.substr(0, port_it);
  RedisCreationSettings settings;
  /// TODO: Do we have any config for this mode?
  /// Here we allow read from replicas possibly stale data
  settings.send_readonly = true;
  auto instance = std::make_shared<Redis>(redis_thread_pool_, settings);
  {
    auto settings_ptr = commands_buffering_settings_.Lock();
    if (settings_ptr->has_value()) {
      instance->SetCommandsBufferingSettings(settings_ptr->value());
    }
  }
  {
    auto settings_ptr = monitoring_settings_.Lock();
    instance->SetReplicationMonitoringSettings(*settings_ptr);
  }

  instance->Connect({host}, port, password_);
  LOG_DEBUG() << "Create new redis instance " << host_port;
  return instance;
}

void ClusterTopologyHolder::UpdateClusterTopology() {
  if (!is_nodes_received_) {
    LOG_DEBUG() << "Skip updating cluster topology: no nodes yet";
    return;
  };
  if (update_cluster_slots_flag_.exchange(true)) return;
  /// Update sentinel
  sentinels_->ProcessCreation(redis_thread_pool_);

  /// Update controlled topology. Go to CLUSTER SLOTS
  /// ...
  ProcessGetClusterHostsRequest(
      shards_names_, GetClusterHostsRequest(*sentinels_, password_),
      [this](ClusterShardHostInfos shard_infos, size_t requests_sent,
             size_t responses_parsed, bool is_non_cluster_error) {
        LOG_DEBUG()
            << "Parsing response from cluster slots: shard_infos.size(): "
            << shard_infos.size() << ", requests_sent=" << requests_sent
            << ", responses_parsed=" << responses_parsed;
        const auto deferred = utils::FastScopeGuard([&]() noexcept {
          update_cluster_slots_flag_ = false;
          ++cluster_slots_call_counter_;
        });
        if (is_non_cluster_error) {
          LOG_DEBUG() << "Non cluster error: shard_infos.size(): "
                      << shard_infos.size();
          throw std::runtime_error("Redis must be in cluster mode");
        }

        if (!CheckQuorum(requests_sent, responses_parsed)) {
          LOG_WARNING()
              << "Too many 'cluster slots' requests failed: requests_sent="
              << requests_sent << " responses_parsed=" << responses_parsed;
          return;
        }

        {
          auto temp_read_ptr = topology_.Read();
          if (temp_read_ptr->HasSameInfos(shard_infos)) {
            /// Nothing new here so do nothing
            return;
          }
        }

        topology_.Assign(ClusterTopology(
            ++current_topology_version_, std::chrono::steady_clock::now(),
            std::move(shard_infos), password_, redis_thread_pool_, nodes_));
        is_topology_received_ = true;

        LOG_DEBUG() << "Cluster topology updated to version"
                    << current_topology_version_.load();
      });
}

void ClusterTopologyHolder::GetStatistics(
    SentinelStatistics& stats, const MetricsSettings& settings) const {
  if (sentinels_) {
    stats.sentinel.emplace(sentinels_->GetStatistics(true, settings));
  }

  auto topology = GetTopology();
  topology->GetStatistics(settings, stats, *shards_names_);
}

ClusterSentinelImpl::ClusterSentinelImpl(
    const engine::ev::ThreadControl& sentinel_thread_control,
    const std::shared_ptr<engine::ev::ThreadPool>& redis_thread_pool,
    Sentinel& sentinel, const std::vector<std::string>& shards,
    const std::vector<ConnectionInfo>& conns, std::string shard_group_name,
    const std::string& client_name, const Password& password,
    ConnectionSecurity /*connection_security*/,
    ReadyChangeCallback ready_callback,
    std::unique_ptr<KeyShard>&& /*key_shard*/,
    dynamic_config::Source dynamic_config_source, ConnectionMode /*mode*/)
    : sentinel_obj_(sentinel),
      ev_thread_(sentinel_thread_control),
      process_waiting_commands_timer_(
          std::make_unique<engine::ev::PeriodicWatcher>(
              ev_thread_, [this] { ProcessWaitingCommands(); },
              kSentinelGetHostsCheckInterval)),
      topology_holder_(std::make_unique<ClusterTopologyHolder>(
          ev_thread_, redis_thread_pool, shard_group_name, password, shards,
          conns)),
      shard_group_name_(std::move(shard_group_name)),
      conns_(conns),
      ready_callback_(std::move(ready_callback)),
      redis_thread_pool_(redis_thread_pool),
      client_name_(client_name),
      password_(password),
      dynamic_config_source_(std::move(dynamic_config_source)) {
  std::ignore = sentinel_obj_;
  Init();
  LOG_DEBUG() << "Created ClusterSentinelImpl, shard_group_name="
              << shard_group_name_;
}

ClusterSentinelImpl::~ClusterSentinelImpl() { Stop(); }

std::unordered_map<ServerId, size_t, ServerIdHasher>
ClusterSentinelImpl::GetAvailableServersWeighted(
    size_t /*shard_idx*/, bool /*with_master*/,
    const CommandControl& /*cc*/) const {
  throw std::runtime_error(std::string(__func__) + " Unimplemented yet");
}

void ClusterSentinelImpl::WaitConnectedDebug(bool /*allow_empty_slaves*/) {
  const RedisWaitConnected wait_connected{WaitConnectedMode::kMasterAndSlave,
                                          false,
                                          kRedisWaitConnectedDefaultTimeout};
  WaitConnectedOnce(wait_connected);
}

void ClusterSentinelImpl::WaitConnectedOnce(RedisWaitConnected wait_connected) {
  auto deadline = engine::Deadline::FromDuration(wait_connected.timeout);
  if (!topology_holder_->WaitReadyOnce(deadline, wait_connected.mode)) {
    const std::string msg = fmt::format(
        "Failed to init cluster slots for redis, shard_group_name={} in {} "
        "ms, mode={}",
        shard_group_name_, wait_connected.timeout.count(),
        ToString(wait_connected.mode));
    if (wait_connected.throw_on_fail)
      throw ClientNotConnectedException(msg);
    else {
      LOG_WARNING() << msg << ", starting with not ready Redis client";
    }
  }
}

void ClusterSentinelImpl::ForceUpdateHosts() {
  throw std::runtime_error(std::string(__func__) + " Unimplemented yet");
}

void ClusterSentinelImpl::Init() { topology_holder_->Init(); }

void ClusterSentinelImpl::AsyncCommand(const SentinelCommand& scommand,
                                       size_t prev_instance_idx) {
  if (!AdjustDeadline(scommand, dynamic_config_source_)) {
    auto reply = std::make_shared<Reply>("", ReplyData::CreateNil());
    reply->status = ReplyStatus::kTimeoutError;
    InvokeCommand(scommand.command, std::move(reply));
    return;
  }

  const CommandPtr command = scommand.command;
  const size_t shard = scommand.shard == kUnknownShard ? 0 : scommand.shard;
  const bool master = scommand.master;
  const auto start = scommand.start;
  const auto counter = command->counter;
  CommandPtr const command_check_errors(PrepareCommand(
      std::move(command->args),
      [this, shard, master, start, counter, command](const CommandPtr& ccommand,
                                                     ReplyPtr reply) {
        if (counter != command->counter) return;
        UASSERT(reply);

        const std::chrono::steady_clock::time_point now =
            std::chrono::steady_clock::now();

        const bool error_ask = reply->data.IsErrorAsk();
        const bool error_moved = reply->data.IsErrorMoved();
        if (error_moved) {
          auto topology = topology_holder_->GetTopology();

          const auto& args = ccommand->args.args;
          LOG_DEBUG() << "MOVED" << reply->status_string
                      << " c.instance_idx:" << ccommand->instance_idx
                      << " shard: " << shard
                      << " movedto:" << ParseMovedShard(reply->data.GetError())
                      << " args:" << args;
          this->topology_holder_->SendUpdateClusterTopology();
        }
        const bool retry_to_master =
            !master && reply->data.IsNil() &&
            command->control.force_retries_to_master_on_nil_reply;
        const bool retry = retry_to_master ||
                           reply->status != ReplyStatus::kOk || error_ask ||
                           error_moved || reply->IsUnusableInstanceError() ||
                           reply->IsReadonlyError();

        std::shared_ptr<Redis> moved_to_instance;
        if (retry) {
          const size_t new_shard = shard;
          size_t retries_left = command->control.max_retries - 1;
          if (error_ask || error_moved) {
            LOG_DEBUG() << "Got error '" << reply->data.GetError()
                        << "' reply, cmd=" << reply->cmd
                        << ", server=" << reply->server_id.GetDescription();
            const auto& host_port = ParseMovedShard(reply->data.GetError());
            command->counter++;
            if (!command->redirected || (error_ask && !command->asking))
              ++retries_left;
            moved_to_instance = topology_holder_->GetRedisInstance(host_port);
            if (!moved_to_instance) {
              LOG_WARNING() << "moved to unknown host " << host_port;
              /// Can we do something else?? We don't have client for this
              /// redis instance
              return;
            }
          }
          const std::chrono::steady_clock::time_point until =
              start + command->control.timeout_all;
          if (now < until && retries_left > 0) {
            const std::chrono::milliseconds timeout_all =
                std::chrono::duration_cast<std::chrono::milliseconds>(until -
                                                                      now);
            auto command_control = command->control;
            command_control.timeout_single =
                std::min(command_control.timeout_single, timeout_all);
            command_control.timeout_all = timeout_all;
            command_control.max_retries = retries_left;

            auto new_command = PrepareCommand(
                std::move(ccommand->args), command->Callback(), command_control,
                command->counter + 1, command->asking || error_ask, 0,
                error_ask || error_moved);
            new_command->log_extra = std::move(command->log_extra);
            if (moved_to_instance) {
              moved_to_instance->AsyncCommand(new_command);
            } else {
              AsyncCommand(
                  SentinelCommand(new_command,
                                  master || retry_to_master ||
                                      (error_moved && shard == new_shard),
                                  new_shard, start),
                  ccommand->instance_idx);
            }
            return;
          }
        }

        const std::chrono::duration<double> time = now - start;
        reply->time = time.count();
        command->args = std::move(ccommand->args);
        InvokeCommand(command, std::move(reply));
        ccommand->args = std::move(command->args);
      },
      command->control, command->counter, command->asking, prev_instance_idx,
      false, !master));

  const auto topology = topology_holder_->GetTopology();
  auto master_shard = topology->GetClusterShardByIndex(shard);
  if (!master_shard.AsyncCommand(command_check_errors)) {
    scommand.command->args = std::move(command_check_errors->args);
    AsyncCommandFailed(scommand);
    return;
  }
}

void ClusterSentinelImpl::AsyncCommandToSentinel(CommandPtr /*command*/) {
  throw std::runtime_error(std::string(__func__) + " Unimplemented yet");
}

size_t ClusterSentinelImpl::ShardByKey(const std::string& key) const {
  const auto slot = HashSlot(key);
  const auto ptr = topology_holder_->GetTopology();
  return ptr->GetShardIndexBySlot(slot);
}

const std::string& ClusterSentinelImpl::GetAnyKeyForShard(
    size_t /*shard_idx*/) const {
  throw std::runtime_error(
      "GetAnyKeyForShard() is not supported in redis cluster mode");
}

void ClusterSentinelImpl::Start() {
  topology_holder_->Start();
  process_waiting_commands_timer_->Start();
}

void ClusterSentinelImpl::AsyncCommandFailed(const SentinelCommand& scommand) {
  // Run command callbacks from redis thread only.
  // It prevents recursive mutex locking in subscription_storage.
  EnqueueCommand(scommand);
}

void ClusterSentinelImpl::Stop() {
  topology_holder_->Stop();
  ev_thread_.RunInEvLoopBlocking(
      [this] { process_waiting_commands_timer_->Stop(); });
  ProcessWaitingCommandsOnStop();
}

std::vector<std::shared_ptr<const Shard>> ClusterSentinelImpl::GetMasterShards()
    const {
  throw std::runtime_error("Unimplemented yet");
  /// just return all Shards
  // return {master_shards_.begin(), master_shards_.end()};
}

bool ClusterSentinelImpl::IsInClusterMode() const { return true; }

void ClusterSentinelImpl::SetCommandsBufferingSettings(
    CommandsBufferingSettings commands_buffering_settings) {
  if (topology_holder_) {
    topology_holder_->SetCommandsBufferingSettings(commands_buffering_settings);
  }
}

void ClusterSentinelImpl::SetReplicationMonitoringSettings(
    const ReplicationMonitoringSettings& monitoring_settings) {
  if (topology_holder_) {
    topology_holder_->SetReplicationMonitoringSettings(monitoring_settings);
  }
}

SentinelStatistics ClusterSentinelImpl::GetStatistics(
    const MetricsSettings& settings) const {
  if (!topology_holder_) {
    return {settings, {}};
  }

  SentinelStatistics stats(settings, statistics_internal_);
  topology_holder_->GetStatistics(stats, settings);
  return stats;
}

void ClusterSentinelImpl::EnqueueCommand(const SentinelCommand& command) {
  const std::lock_guard<std::mutex> lock(command_mutex_);
  commands_.push_back(command);
}

size_t ClusterSentinelImpl::ShardsCount() const {
  const auto ptr = topology_holder_->GetTopology();
  return ptr->GetShardsCount();
}

size_t ClusterSentinelImpl::GetClusterSlotsCalledCounter() {
  return ClusterTopologyHolder::GetClusterSlotsCalledCounter();
}

}  // namespace redis

USERVER_NAMESPACE_END
