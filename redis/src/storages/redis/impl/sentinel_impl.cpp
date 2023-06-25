#include <storages/redis/impl/sentinel_impl.hpp>

#include <algorithm>
#include <sstream>
#include <thread>

#include <boost/algorithm/string.hpp>
#include <boost/crc.hpp>

#include <fmt/format.h>

#include <hiredis/hiredis.h>

#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>

#include <storages/redis/dynamic_config.hpp>
#include <storages/redis/impl/command.hpp>
#include <storages/redis/impl/keyshard_impl.hpp>
#include <storages/redis/impl/sentinel.hpp>
#include <userver/server/request/task_inherited_data.hpp>
#include <userver/storages/redis/impl/exception.hpp>
#include <userver/storages/redis/impl/reply.hpp>
#include <userver/utils/impl/userver_experiments.hpp>

USERVER_NAMESPACE_BEGIN

namespace redis {
namespace {

utils::impl::UserverExperiment kDeadlinePropagationExperiment{
    "redis-deadline-propagation", true};

bool CheckQuorum(size_t requests_sent, size_t responses_parsed) {
  size_t quorum = requests_sent / 2 + 1;
  return responses_parsed >= quorum;
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

}  // namespace

SentinelImpl::SentinelImpl(
    const engine::ev::ThreadControl& sentinel_thread_control,
    const std::shared_ptr<engine::ev::ThreadPool>& redis_thread_pool,
    Sentinel& sentinel, const std::vector<std::string>& shards,
    const std::vector<ConnectionInfo>& conns, std::string shard_group_name,
    const std::string& client_name, const Password& password,
    ConnectionSecurity connection_security, ReadyChangeCallback ready_callback,
    std::unique_ptr<KeyShard>&& key_shard,
    dynamic_config::Source dynamic_config_source, ConnectionMode mode)
    : sentinel_obj_(sentinel),
      ev_thread_(sentinel_thread_control),
      shard_group_name_(std::move(shard_group_name)),
      init_shards_(std::make_shared<const std::vector<std::string>>(shards)),
      conns_(conns),
      ready_callback_(std::move(ready_callback)),
      redis_thread_pool_(redis_thread_pool),
      connection_security_(connection_security),
      check_interval_(ToEvDuration(kSentinelGetHostsCheckInterval)),
      update_cluster_slots_flag_(false),
      cluster_mode_failed_(false),
      key_shard_(std::move(key_shard)),
      connection_mode_(mode),
      slot_info_(IsInClusterMode() ? std::make_unique<SlotInfo>() : nullptr),
      dynamic_config_source_(dynamic_config_source) {
  for (size_t i = 0; i < init_shards_->size(); ++i) {
    shards_[(*init_shards_)[i]] = i;
    connected_statuses_.push_back(std::make_unique<ConnectedStatus>());
  }
  client_name_ = client_name;
  password_ = password;

  Init();
  InitKeyShard();
  LOG_DEBUG() << "Created SentinelImpl, shard_group_name=" << shard_group_name_
              << ", cluster_mode=" << IsInClusterMode();
}

SentinelImpl::~SentinelImpl() { Stop(); }

std::unordered_map<ServerId, size_t, ServerIdHasher>
SentinelImpl::GetAvailableServersWeighted(size_t shard_idx, bool with_master,
                                          const CommandControl& cc) const {
  return master_shards_.at(shard_idx)->GetAvailableServersWeighted(with_master,
                                                                   cc);
}

void SentinelImpl::WaitConnectedDebug(bool allow_empty_slaves) {
  static const auto timeout = std::chrono::milliseconds(50);

  for (;; std::this_thread::sleep_for(timeout)) {
    std::lock_guard<std::mutex> sentinels_lock(sentinels_mutex_);

    bool connected_all = true;
    for (const auto& shard : master_shards_)
      connected_all = connected_all &&
                      shard->IsConnectedToAllServersDebug(allow_empty_slaves);
    if (connected_all && (!IsInClusterMode() || slot_info_->IsInitialized()))
      return;
  }
}

void SentinelImpl::WaitConnectedOnce(RedisWaitConnected wait_connected) {
  auto deadline = engine::Deadline::FromDuration(wait_connected.timeout);

  for (size_t i = 0; i < connected_statuses_.size(); ++i) {
    auto& shard = *connected_statuses_[i];
    if (!shard.WaitReady(deadline, wait_connected.mode)) {
      std::string msg =
          "Failed to connect to redis, shard_group_name=" + shard_group_name_ +
          ", shard=" + (*init_shards_)[i] + " in " +
          std::to_string(wait_connected.timeout.count()) +
          " ms, mode=" + ToString(wait_connected.mode);
      if (wait_connected.throw_on_fail)
        throw ClientNotConnectedException(msg);
      else
        LOG_ERROR() << msg << ", starting with not ready Redis client";
    }
  }
  if (IsInClusterMode()) {
    if (!slot_info_->WaitInitialized(deadline)) {
      std::string msg = fmt::format(
          "Failed to init cluster slots for redis, shard_group_name={} in {} "
          "ms, mode={}",
          shard_group_name_, wait_connected.timeout.count(),
          ToString(wait_connected.mode));
      if (wait_connected.throw_on_fail)
        throw ClientNotConnectedException(msg);
      else
        LOG_WARNING() << msg << ", starting with not ready Redis client";
    }
  }
}

void SentinelImpl::ForceUpdateHosts() { ev_thread_.Send(watch_create_); }

void SentinelImpl::Init() {
  InitShards(*init_shards_, master_shards_, ready_callback_);

  Shard::Options shard_options;
  shard_options.shard_name = "(sentinel)";
  shard_options.shard_group_name = shard_group_name_;
  shard_options.cluster_mode = IsInClusterMode();
  shard_options.ready_change_callback = [this](bool ready) {
    if (ready) ev_thread_.Send(watch_create_);
  };
  shard_options.connection_infos = conns_;
  sentinels_ = std::make_shared<Shard>(std::move(shard_options));
  sentinels_->SignalInstanceStateChange().connect(
      [this](ServerId id, Redis::State state) {
        LOG_TRACE() << "Signaled server " << id.GetDescription()
                    << " state=" << Redis::StateToString(state);
        if (state != Redis::State::kInit) ev_thread_.Send(watch_state_);
      });
  sentinels_->SignalNotInClusterMode().connect([this]() {
    cluster_mode_failed_ = true;
    ev_thread_.Send(watch_state_);
  });
}

constexpr const std::chrono::milliseconds SentinelImpl::cluster_slots_timeout_;

void SentinelImpl::InitShards(
    const std::vector<std::string>& shards,
    std::vector<std::shared_ptr<Shard>>& shard_objects,
    const ReadyChangeCallback& ready_callback) {
  size_t i = 0;
  shard_objects.clear();
  for (const auto& shard : shards) {
    Shard::Options shard_options;
    shard_options.shard_name = shard;
    shard_options.shard_group_name = shard_group_name_;
    shard_options.cluster_mode = IsInClusterMode();
    shard_options.ready_change_callback = [i, shard,
                                           ready_callback](bool ready) {
      if (ready_callback) ready_callback(i, shard, ready);
    };
    auto object = std::make_shared<Shard>(std::move(shard_options));
    object->SignalInstanceStateChange().connect(
        [this](ServerId, Redis::State state) {
          if (state != Redis::State::kInit) ev_thread_.Send(watch_state_);
        });
    object->SignalInstanceReady().connect([this, i](ServerId, bool read_only) {
      LOG_TRACE() << "Signaled kConnected to sentinel: shard_idx=" << i
                  << ", master=" << !read_only;
      if (!read_only)
        connected_statuses_[i]->SetMasterReady();
      else
        connected_statuses_[i]->SetSlaveReady();
    });
    shard_objects.emplace_back(std::move(object));
    ++i;
  }
}

static inline void InvokeCommand(CommandPtr command, ReplyPtr&& reply) {
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

std::optional<std::chrono::milliseconds> GetDeadlineTimeLeft(
    const dynamic_config::Snapshot& config) {
  if (!engine::current_task::IsTaskProcessorThread()) return std::nullopt;

  if (!kDeadlinePropagationExperiment.IsEnabled()) return std::nullopt;

  if (config[kDeadlinePropagationVersion] !=
      kDeadlinePropagationExperimentVersion) {
    return std::nullopt;
  }

  const auto inherited_deadline = server::request::GetTaskInheritedDeadline();
  if (inherited_deadline.IsReachable()) {
    const auto inherited_timeout =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            inherited_deadline.TimeLeftApprox());
    return inherited_timeout;
  }
  return std::nullopt;
}

bool SentinelImplBase::AdjustDeadline(
    const SentinelCommand& scommand,
    const dynamic_config::Source& dynamic_config_source) {
  const auto inherited_deadline =
      GetDeadlineTimeLeft(dynamic_config_source.GetSnapshot());
  if (!inherited_deadline) return true;

  if (*inherited_deadline <= std::chrono::seconds{0}) {
    return false;
  }

  auto& command = *scommand.command;
  auto& cc = command.control;
  cc.timeout_single = std::min(cc.timeout_single, *inherited_deadline);
  cc.timeout_all = std::min(cc.timeout_all, *inherited_deadline);

  return true;
}

void SentinelImpl::AsyncCommand(const SentinelCommand& scommand,
                                size_t prev_instance_idx) {
  if (!AdjustDeadline(scommand, dynamic_config_source_)) {
    auto reply = std::make_shared<Reply>("", ReplyData::CreateNil());
    reply->status = ReplyStatus::kTimeoutError;
    InvokeCommand(scommand.command, std::move(reply));
    return;
  }

  CommandPtr command = scommand.command;
  size_t shard = scommand.shard;
  bool master = scommand.master;

  if (shard == kUnknownShard) shard = 0;

  auto start = scommand.start;
  auto counter = command->counter;
  CommandPtr command_check_errors(PrepareCommand(
      std::move(command->args),
      [this, shard, master, start, counter, command](const CommandPtr& ccommand,
                                                     ReplyPtr reply) {
        if (counter != command->counter) return;
        UASSERT(reply);

        std::chrono::steady_clock::time_point now =
            std::chrono::steady_clock::now();

        bool error_ask = reply->data.IsErrorAsk();
        bool error_moved = reply->data.IsErrorMoved();
        if (error_moved) RequestUpdateClusterSlots(shard);
        bool retry_to_master =
            !master && reply->data.IsNil() &&
            command->control.force_retries_to_master_on_nil_reply;
        bool retry = retry_to_master || reply->status != ReplyStatus::kOk ||
                     error_ask || error_moved ||
                     reply->IsUnusableInstanceError() ||
                     reply->IsReadonlyError();

        if (retry) {
          size_t new_shard = shard;
          size_t retries_left = command->control.max_retries - 1;
          if (error_ask || error_moved) {
            LOG_DEBUG() << "Got error '" << reply->data.GetError()
                        << "' reply, cmd=" << reply->cmd
                        << ", server=" << reply->server_id.GetDescription();
            new_shard = ParseMovedShard(
                reply->data
                    .GetError());  // TODO: use correct host:port from shard
            command->counter++;
            if (!command->redirected || (error_ask && !command->asking))
              ++retries_left;
          }
          std::chrono::steady_clock::time_point until =
              start + command->control.timeout_all;
          if (now < until && retries_left > 0) {
            std::chrono::milliseconds timeout_all =
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
            AsyncCommand(
                SentinelCommand(new_command,
                                master || retry_to_master ||
                                    (error_moved && shard == new_shard),
                                new_shard, start),
                ccommand->instance_idx);
            return;
          }
        }

        std::chrono::duration<double> time = now - start;
        reply->time = time.count();
        command->args = std::move(ccommand->args);
        InvokeCommand(command, std::move(reply));
        ccommand->args = std::move(command->args);
      },
      command->control, command->counter, command->asking, prev_instance_idx,
      false, !master));

  UASSERT(shard < master_shards_.size());
  auto master_shard = master_shards_[shard];
  if (!master_shard->AsyncCommand(command_check_errors)) {
    scommand.command->args = std::move(command_check_errors->args);
    AsyncCommandFailed(scommand);
    return;
  }
}

void SentinelImpl::AsyncCommandToSentinel(CommandPtr command) {
  UASSERT(sentinels_);
  sentinels_->AsyncCommand(std::move(command));
}

size_t SentinelImpl::ShardByKey(const std::string& key) const {
  UASSERT(!master_shards_.empty());
  auto key_shard = key_shard_.Get();
  size_t shard = key_shard ? key_shard->ShardByKey(key)
                           : slot_info_->ShardBySlot(HashSlot(key));
  LOG_TRACE() << "key=" << key << " shard=" << shard;
  return shard;
}

const std::string& SentinelImpl::GetAnyKeyForShard(size_t shard_idx) const {
  if (IsInClusterMode())
    throw std::runtime_error(
        "GetAnyKeyForShard() is not supported in redis cluster mode");
  auto keys_for_shards = keys_for_shards_.Get();
  if (!keys_for_shards)
    throw std::runtime_error(
        "keys were not generated with GenerateKeysForShards()");
  return keys_for_shards->GetAnyKeyForShard(shard_idx);
}

void SentinelImpl::Start() {
  watch_state_.data = this;
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
  ev_async_init(&watch_state_, ChangedState);
  ev_thread_.Start(watch_state_);

  watch_update_.data = this;
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
  ev_async_init(&watch_update_, UpdateInstances);
  ev_thread_.Start(watch_update_);

  watch_create_.data = this;
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
  ev_async_init(&watch_create_, OnModifyConnectionInfo);
  ev_thread_.Start(watch_create_);

  if (IsInClusterMode()) {
    watch_cluster_slots_.data = this;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
    ev_async_init(&watch_cluster_slots_, OnUpdateClusterSlotsRequested);
    ev_thread_.Start(watch_cluster_slots_);
  }

  check_timer_.data = this;
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
  ev_timer_init(&check_timer_, OnCheckTimer, 0.0, 0.0);
  ev_thread_.Start(check_timer_);
}

void SentinelImpl::InitKeyShard() {
  auto key_shard = key_shard_.Get();
  if (key_shard) {
    try {
      if (key_shard->IsGenerateKeysForShardsEnabled()) GenerateKeysForShards();
    } catch (const std::exception& ex) {
      LOG_ERROR() << "GenerateKeysForShards() failed: " << ex
                  << ", shard_group_name=" << shard_group_name_;
    }
  }
}

void SentinelImpl::GenerateKeysForShards(size_t max_len) {
  if (IsInClusterMode())
    throw std::runtime_error(
        "GenerateKeysForShards() is not supported in redis cluster mode");
  keys_for_shards_.Set(std::make_shared<KeysForShards>(
      ShardsCount(), [this](const std::string& key) { return ShardByKey(key); },
      max_len));
}

void SentinelImpl::AsyncCommandFailed(const SentinelCommand& scommand) {
  // Run command callbacks from redis thread only.
  // It prevents recursive mutex locking in subscription_storage.
  EnqueueCommand(scommand);
}

void SentinelImpl::OnCheckTimer(struct ev_loop*, ev_timer* w, int) noexcept {
  auto* impl = static_cast<SentinelImpl*>(w->data);
  UASSERT(impl != nullptr);
  impl->RefreshConnectionInfo();
}

void SentinelImpl::ChangedState(struct ev_loop*, ev_async* w, int) noexcept {
  auto* impl = static_cast<SentinelImpl*>(w->data);
  UASSERT(impl != nullptr);
  impl->CheckConnections();
}

void SentinelImpl::ProcessCreationOfShards(
    std::vector<std::shared_ptr<Shard>>& shards) {
  for (size_t shard_idx = 0; shard_idx < shards.size(); shard_idx++) {
    auto& info = *shards[shard_idx];
    if (info.ProcessCreation(redis_thread_pool_)) {
      sentinel_obj_.signal_instances_changed(shard_idx);
    }
  }
}

void SentinelImpl::RefreshConnectionInfo() {
  try {
    sentinels_->ProcessCreation(redis_thread_pool_);

    ProcessCreationOfShards(master_shards_);
    if (IsInClusterMode())
      ReadClusterHosts();
    else
      ReadSentinels();
  } catch (const std::exception& ex) {
    LOG_WARNING() << "exception in RefreshConnectionInfo: " << ex.what();
  }

  /* FIXME: this should be called not every check_interval_,
   * but using a min deadline time of all waiting commands.
   * Replies will be handled in 3 seconds instead of lower timeout with current
   * logic.
   * */
  ProcessWaitingCommands();

  ev_thread_.Stop(check_timer_);
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
  ev_timer_set(&check_timer_, check_interval_, 0.0);
  ev_thread_.Start(check_timer_);
}

void SentinelImpl::Stop() {
  ev_thread_.RunInEvLoopBlocking([this] {
    ev_thread_.Stop(check_timer_);
    ev_thread_.Stop(watch_state_);
    ev_thread_.Stop(watch_update_);
    ev_thread_.Stop(watch_create_);
    if (IsInClusterMode()) ev_thread_.Stop(watch_cluster_slots_);

    auto clean_shards = [](std::vector<std::shared_ptr<Shard>>& shards) {
      for (auto& shard : shards) shard->Clean();
    };
    {
      std::lock_guard<std::mutex> lock(command_mutex_);
      while (!commands_.empty()) {
        auto command = commands_.back().command;
        for (const auto& args : command->args.args) {
          LOG_ERROR() << "Killing request: " << boost::join(args, ", ");
          auto reply = std::make_shared<Reply>(
              args[0], nullptr, ReplyStatus::kEndOfFileError,
              "Stopping, killing commands remaining in send queue");
          statistics_internal_.redis_not_ready++;
          InvokeCommand(command, std::move(reply));
        }
        commands_.pop_back();
      }
    }
    clean_shards(master_shards_);
    sentinels_->Clean();
  });
}

std::vector<std::shared_ptr<const Shard>> SentinelImpl::GetMasterShards()
    const {
  return {master_shards_.begin(), master_shards_.end()};
}

bool SentinelImpl::IsInClusterMode() const { return !key_shard_.Get(); }

void SentinelImpl::SetCommandsBufferingSettings(
    CommandsBufferingSettings commands_buffering_settings) {
  if (commands_buffering_settings_ &&
      *commands_buffering_settings_ == commands_buffering_settings)
    return;
  commands_buffering_settings_ = commands_buffering_settings;
  for (auto& shard : master_shards_)
    shard->SetCommandsBufferingSettings(commands_buffering_settings);
}

void SentinelImpl::SetReplicationMonitoringSettings(
    const ReplicationMonitoringSettings& replication_monitoring_settings) {
  for (auto& shard : master_shards_)
    shard->SetReplicationMonitoringSettings(replication_monitoring_settings);
}

void SentinelImpl::RequestUpdateClusterSlots(size_t shard) {
  current_slots_shard_ = shard;
  ev_thread_.Send(watch_cluster_slots_);
}

void SentinelImpl::UpdateClusterSlots(size_t shard) {
  if (update_cluster_slots_flag_.exchange(true)) return;
  LOG_TRACE() << "UpdateClusterSlots, shard=" << shard;
  CommandPtr command =
      PrepareCommand(CmdArgs{"CLUSTER", "SLOTS"},
                     [this](const CommandPtr&, ReplyPtr reply) {
                       DoUpdateClusterSlots(reply);
                       update_cluster_slots_flag_ = false;
                     },
                     {cluster_slots_timeout_, cluster_slots_timeout_, 1});

  std::shared_ptr<Shard> master;
  if (shard < master_shards_.size()) master = master_shards_[shard];
  if (!master || !master->AsyncCommand(command))
    update_cluster_slots_flag_ = false;
}

void SentinelImpl::DoUpdateClusterSlots(ReplyPtr reply) {
  UASSERT(reply);
  LOG_TRACE() << "Got reply to CLUSTER SLOTS: " << reply->data.ToDebugString();
  if (!reply->data.IsArray()) return;
  std::vector<SlotInfo::ShardInterval> shard_intervals;
  shard_intervals.reserve(reply->data.GetArray().size());
  for (const ReplyData& reply_interval : reply->data.GetArray()) {
    if (!reply_interval.IsArray()) return;
    const std::vector<ReplyData>& array = reply_interval.GetArray();
    if (array.size() < 3) return;
    if (!array[0].IsInt() || !array[1].IsInt()) return;
    for (size_t i = 2; i < array.size(); i++) {
      if (!array[i].IsArray()) return;
      const std::vector<ReplyData>& host_info_array = array[i].GetArray();
      if (host_info_array.size() < 2) return;
      if (!host_info_array[0].IsString() || !host_info_array[1].IsInt()) return;
      size_t shard = shard_info_.GetShard(host_info_array[0].GetString(),
                                          host_info_array[1].GetInt());
      if (shard != kUnknownShard) {
        shard_intervals.emplace_back(array[0].GetInt(), array[1].GetInt(),
                                     shard);
        break;
      }
    }
  }
  slot_info_->UpdateSlots(shard_intervals);
}

void SentinelImpl::ReadSentinels() {
  ProcessGetHostsRequest(
      GetHostsRequest(*sentinels_, password_),
      [this](const ConnInfoByShard& info, size_t requests_sent,
             size_t responses_parsed) {
        if (!CheckQuorum(requests_sent, responses_parsed)) {
          LOG_WARNING()
              << "Too many 'sentinel masters' requests failed: requests_sent="
              << requests_sent << " responses_parsed=" << responses_parsed;
          return;
        }
        struct WatchContext {
          ConnInfoByShard masters;
          ConnInfoByShard slaves;
          std::mutex mutex;
          int counter{0};
          ShardInfo::HostPortToShardMap host_port_to_shard;
        };
        auto watcher = std::make_shared<WatchContext>();

        std::vector<uint8_t> shard_found(init_shards_->size(), false);
        for (auto shard_conn : info) {
          if (shards_.find(shard_conn.Name()) != shards_.end()) {
            shard_conn.SetConnectionSecurity(connection_security_);
            shard_found[shards_[shard_conn.Name()]] = true;
            watcher->host_port_to_shard[shard_conn.HostPort()] =
                shards_[shard_conn.Name()];
            watcher->masters.push_back(std::move(shard_conn));
          }
        }

        for (size_t idx = 0; idx < shard_found.size(); idx++) {
          if (!shard_found[idx]) {
            LOG_WARNING() << "Shard with name=" << (*init_shards_)[idx]
                          << " was not found in 'SENTINEL MASTERS' reply for "
                             "shard_group_name="
                          << shard_group_name_
                          << ". If problem persists, check service and "
                             "redis-sentinel configs.";
          }
        }
        watcher->counter = watcher->masters.size();

        for (const auto& shard_conn : watcher->masters) {
          const auto& shard = shard_conn.Name();
          ProcessGetHostsRequest(
              GetHostsRequest(*sentinels_, shard_conn.Name(), password_),
              [this, watcher, shard](const ConnInfoByShard& info,
                                     size_t requests_sent,
                                     size_t responses_parsed) {
                if (!CheckQuorum(requests_sent, responses_parsed)) {
                  LOG_WARNING() << "Too many 'sentinel slaves' requests "
                                   "failed: requests_sent="
                                << requests_sent
                                << " responses_parsed=" << responses_parsed;
                  return;
                }
                std::lock_guard<std::mutex> lock(watcher->mutex);
                for (auto shard_conn : info) {
                  shard_conn.SetName(shard);
                  shard_conn.SetReadOnly(true);
                  shard_conn.SetConnectionSecurity(connection_security_);
                  if (shards_.find(shard_conn.Name()) != shards_.end())
                    watcher->host_port_to_shard[shard_conn.HostPort()] =
                        shards_[shard_conn.Name()];
                  watcher->slaves.push_back(std::move(shard_conn));
                }
                if (!--watcher->counter) {
                  shard_info_.UpdateHostPortToShard(
                      std::move(watcher->host_port_to_shard));

                  {
                    std::lock_guard<std::mutex> lock_this(sentinels_mutex_);
                    master_shards_info_.swap(watcher->masters);
                    slaves_shards_info_.swap(watcher->slaves);
                  }
                  ev_thread_.Send(watch_update_);
                }
              });
        }
      });
}

void SentinelImpl::ReadClusterHosts() {
  ProcessGetClusterHostsRequest(
      init_shards_, GetClusterHostsRequest(*sentinels_, password_),
      [this](ClusterShardHostInfos shard_infos, size_t requests_sent,
             size_t responses_parsed, bool is_non_cluster_error) {
        if (is_non_cluster_error) {
          cluster_mode_failed_ = true;
          ev_thread_.Send(watch_state_);
          return;
        }
        if (!CheckQuorum(requests_sent, responses_parsed)) {
          LOG_WARNING()
              << "Too many 'cluster slots' requests failed: requests_sent="
              << requests_sent << " responses_parsed=" << responses_parsed;
          return;
        }
        if (shard_infos.empty()) return;

        std::vector<SlotInfo::ShardInterval> shard_intervals;
        for (const auto& shard_info : shard_infos) {
          for (const auto& interval : shard_info.slot_intervals) {
            shard_intervals.emplace_back(interval.slot_min, interval.slot_max,
                                         shards_[shard_info.master.Name()]);
          }
        }

        ConnInfoByShard masters;
        ConnInfoByShard slaves;
        ShardInfo::HostPortToShardMap host_port_to_shard;

        for (auto& shard_info : shard_infos) {
          host_port_to_shard[shard_info.master.HostPort()] =
              shards_[shard_info.master.Name()];
          masters.push_back(std::move(shard_info.master));
          for (auto& slave_info : shard_info.slaves) {
            slave_info.SetReadOnly(true);
            host_port_to_shard[slave_info.HostPort()] =
                shards_[slave_info.Name()];
            slaves.push_back(std::move(slave_info));
          }
        }
        shard_info_.UpdateHostPortToShard(std::move(host_port_to_shard));

        {
          std::lock_guard<std::mutex> lock(sentinels_mutex_);
          master_shards_info_.swap(masters);
          slaves_shards_info_.swap(slaves);
        }
        ev_thread_.Send(watch_update_);

        slot_info_->UpdateSlots(shard_intervals);
      });
}

void SentinelImpl::UpdateInstances(struct ev_loop*, ev_async* w, int) noexcept {
  auto* impl = static_cast<SentinelImpl*>(w->data);
  UASSERT(impl != nullptr);
  impl->UpdateInstancesImpl();
}

bool SentinelImpl::SetConnectionInfo(
    ConnInfoMap info_by_shards, std::vector<std::shared_ptr<Shard>>& shards) {
  /* Ensure all shards are in info_by_shards to remove the last instance
   * from the empty shard */
  for (const auto& it : shards_) {
    info_by_shards[it.first];
  }

  bool res = false;
  for (const auto& info_iterator : info_by_shards) {
    auto j = shards_.find(info_iterator.first);
    if (j != shards_.end()) {
      size_t shard = j->second;
      auto shard_ptr = shards[shard];
      bool changed = shard_ptr->SetConnectionInfo(info_iterator.second);

      if (changed) {
        std::vector<std::string> conn_strs;
        for (const auto& conn_str : info_iterator.second)
          conn_strs.push_back(conn_str.Fulltext());
        LOG_INFO() << "Redis state changed for client=" << client_name_
                   << " shard=" << j->first << ", now it is "
                   << boost::join(conn_strs, ", ")
                   << ", connections=" << shard_ptr->InstancesSize();
        res = true;
      }
    }
  }
  return res;
}

void SentinelImpl::UpdateInstancesImpl() {
  bool changed = false;
  {
    std::lock_guard<std::mutex> lock(sentinels_mutex_);
    ConnInfoMap info_map;
    for (const auto& info : master_shards_info_)
      info_map[info.Name()].emplace_back(info);
    for (const auto& info : slaves_shards_info_)
      info_map[info.Name()].emplace_back(info);
    changed |= SetConnectionInfo(std::move(info_map), master_shards_);
  }
  if (changed) ev_thread_.Send(watch_create_);
}

void SentinelImpl::OnModifyConnectionInfo(struct ev_loop*, ev_async* w,
                                          int) noexcept {
  auto* impl = static_cast<SentinelImpl*>(w->data);
  UASSERT(impl != nullptr);
  impl->RefreshConnectionInfo();
}

void SentinelImpl::OnUpdateClusterSlotsRequested(struct ev_loop*, ev_async* w,
                                                 int) noexcept {
  auto* impl = static_cast<SentinelImpl*>(w->data);
  UASSERT(impl != nullptr);
  impl->UpdateClusterSlots(impl->current_slots_shard_);
}

void SentinelImpl::CheckConnections() {
  if (cluster_mode_failed_) {
    if (IsInClusterMode()) {
      Stop();
      for (auto& conn : conns_) {
        // In a cluster mode we use masters and slaves as sentinels.
        // So we need a password for such usage.
        // When we switch to non-cluster mode we should connect to sentinels
        // which has no auth with a password.
        // TODO: Since redis 5.0.1 it is possible to set a password for
        // sentinels.
        // TODO: Add `sentinel_password` parameter to redis config and remove
        // this password reset (https://st.yandex-team.ru/TAXICOMMON-2687).
        conn.password = Password("");
      }
      LOG_WARNING() << "Failed to start in redis cluster mode for client="
                    << client_name_ << ". Switch to "
                    << (connection_mode_ == ConnectionMode::kSubscriber
                            ? KeyShardZero::kName
                            : KeyShardCrc32::kName)
                    << " sharding strategy (without redis cluster support).";
      if (connection_mode_ == ConnectionMode::kSubscriber)
        key_shard_.Set(std::make_shared<KeyShardZero>());
      else
        key_shard_.Set(std::make_shared<KeyShardCrc32>(ShardsCount()));
      Init();
      Start();
      InitKeyShard();
      sentinel_obj_.signal_not_in_cluster_mode();
      return;
    }
  }
  sentinels_->ProcessStateUpdate();

  for (size_t shard_idx = 0; shard_idx < master_shards_.size(); shard_idx++) {
    const auto& info = master_shards_[shard_idx];
    if (info->ProcessStateUpdate()) {
      sentinel_obj_.signal_instances_changed(shard_idx);
    }
  }

  ProcessWaitingCommands();
}

void SentinelImpl::ProcessWaitingCommands() {
  std::vector<SentinelCommand> waiting_commands;

  {
    std::lock_guard<std::mutex> lock(command_mutex_);
    waiting_commands.swap(commands_);
  }
  if (!waiting_commands.empty()) {
    LOG_INFO() << "ProcessWaitingCommands client=" << client_name_
               << " shard_group_name=" << shard_group_name_
               << " waiting_commands.size()=" << waiting_commands.size();
  }

  std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
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

SentinelStatistics SentinelImpl::GetStatistics(
    const MetricsSettings& settings) const {
  SentinelStatistics stats(settings, statistics_internal_);
  std::lock_guard<std::mutex> lock(sentinels_mutex_);
  for (const auto& shard : master_shards_) {
    if (!shard) continue;
    auto master_stats = shard->GetStatistics(true, settings);
    auto slave_stats = shard->GetStatistics(false, settings);
    stats.shard_group_total.Add(master_stats.shard_total);
    stats.shard_group_total.Add(slave_stats.shard_total);
    stats.masters.emplace(shard->ShardName(), std::move(master_stats));
    stats.slaves.emplace(shard->ShardName(), std::move(slave_stats));
  }
  stats.sentinel.emplace(sentinels_->GetStatistics(true, settings));
  return stats;
}

void SentinelImpl::EnqueueCommand(const SentinelCommand& command) {
  std::lock_guard<std::mutex> lock(command_mutex_);
  commands_.push_back(command);
}

size_t SentinelImpl::ParseMovedShard(const std::string& err_string) {
  size_t pos = err_string.find(' ');  // skip "MOVED" or "ASK"
  if (pos == std::string::npos) return kUnknownShard;
  pos = err_string.find(' ', pos + 1);  // skip hash_slot
  if (pos == std::string::npos) return kUnknownShard;
  pos++;
  size_t end = err_string.find(' ', pos);
  if (end == std::string::npos) end = err_string.size();
  size_t colon_pos = err_string.rfind(':', end);
  int port = 0;
  try {
    port = std::stoi(err_string.substr(colon_pos + 1, end - (colon_pos + 1)));
  } catch (const std::exception& ex) {
    LOG_WARNING() << "exception in " << __func__ << "(\"" << err_string
                  << "\") " << ex.what();
    return kUnknownShard;
  }
  std::string host = err_string.substr(pos, colon_pos - pos);
  return shard_info_.GetShard(host, port);
}

size_t SentinelImpl::HashSlot(const std::string& key) {
  size_t start = 0;
  size_t len = 0;
  GetRedisKey(key, &start, &len);
  return std::for_each(key.data() + start, key.data() + start + len,
                       boost::crc_optimal<16, 0x1021>())() &
         0x3fff;
}

SentinelImpl::SlotInfo::SlotInfo() {
  for (size_t i = 0; i < kClusterHashSlots; ++i) {
    slot_to_shard_[i] = kUnknownShard;
  }
}

size_t SentinelImpl::SlotInfo::ShardBySlot(size_t slot) const {
  return slot_to_shard_.at(slot);
}

void SentinelImpl::SlotInfo::UpdateSlots(
    const std::vector<ShardInterval>& intervals) {
  [[maybe_unused]] auto check_intervals = [&intervals]() {
    std::vector<size_t> slot_bounds;
    slot_bounds.reserve(intervals.size() * 2);
    for (const ShardInterval& interval : intervals) {
      if (interval.shard == kUnknownShard) continue;
      slot_bounds.push_back(interval.slot_min);
      slot_bounds.push_back(interval.slot_max + 1);
    }
    std::sort(slot_bounds.begin(), slot_bounds.end());
    slot_bounds.erase(std::unique(slot_bounds.begin(), slot_bounds.end()),
                      slot_bounds.end());

    for (const ShardInterval& interval : intervals) {
      size_t idx = std::lower_bound(slot_bounds.begin(), slot_bounds.end(),
                                    interval.slot_min) -
                   slot_bounds.begin();
      if (idx + 1 >= slot_bounds.size() ||
          slot_bounds[idx] != interval.slot_min ||
          slot_bounds[idx + 1] != interval.slot_max + 1) {
        std::ostringstream os;
        os << "wrong shard intervals: [";
        bool f = true;
        for (const ShardInterval& i : intervals) {
          if (f)
            f = false;
          else
            os << ", ";
          os << "{ slot_min = " << i.slot_min << ", "
             << "slot_max = " << i.slot_max << ", "
             << "shard = " << i.shard << "}";
        }
        os << "]";
        LOG_WARNING() << os.str();
        return false;
      }
    }
    return true;
  };
  UASSERT(check_intervals());

  size_t changed_slots = 0;
  for (const auto& interval : intervals) {
    UASSERT(interval.shard != kUnknownShard);
    LOG_TRACE() << "interval: slot_min=" << interval.slot_min
                << " slot_max=" << interval.slot_max
                << " shard=" << interval.shard;
    for (size_t slot = interval.slot_min; slot <= interval.slot_max; ++slot) {
      auto prev_shard = slot_to_shard_[slot].exchange(interval.shard);
      if (prev_shard != interval.shard) ++changed_slots;
    }
  }
  if (changed_slots) {
    LOG_INFO() << "Cluster slots were updated, shard was changed for "
               << changed_slots << " slot(s)";
  }

  if (!is_initialized_.exchange(true)) {
    std::lock_guard<std::mutex> lock(mutex_);
    cv_.NotifyAll();
  }
}

bool SentinelImpl::SlotInfo::IsInitialized() const { return is_initialized_; }

bool SentinelImpl::SlotInfo::WaitInitialized(engine::Deadline deadline) {
  std::unique_lock<std::mutex> lock(mutex_);
  return cv_.WaitUntil(lock, deadline, [this]() { return IsInitialized(); });
}

size_t SentinelImpl::ShardInfo::GetShard(const std::string& host,
                                         int port) const {
  std::lock_guard<std::mutex> lock(mutex_);

  const auto it = host_port_to_shard_.find(std::make_pair(host, port));
  if (it == host_port_to_shard_.end()) return kUnknownShard;
  return it->second;
}

void SentinelImpl::ShardInfo::UpdateHostPortToShard(
    HostPortToShardMap&& host_port_to_shard_new) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (host_port_to_shard_new != host_port_to_shard_) {
    host_port_to_shard_.swap(host_port_to_shard_new);
  }
}

void SentinelImpl::ConnectedStatus::SetMasterReady() {
  if (!master_ready_.exchange(true)) {
    cv_.NotifyAll();
  }
}

void SentinelImpl::ConnectedStatus::SetSlaveReady() {
  if (!slave_ready_.exchange(true)) {
    cv_.NotifyAll();
  }
}

bool SentinelImpl::ConnectedStatus::WaitReady(engine::Deadline deadline,
                                              WaitConnectedMode mode) {
  switch (mode) {
    case WaitConnectedMode::kNoWait:
      return true;
    case WaitConnectedMode::kMaster:
      return Wait(deadline, [this] { return master_ready_.load(); });
    case WaitConnectedMode::kSlave:
      return Wait(deadline, [this] { return slave_ready_.load(); });
    case WaitConnectedMode::kMasterOrSlave:
      return Wait(deadline, [this] { return master_ready_ || slave_ready_; });
    case WaitConnectedMode::kMasterAndSlave:
      return Wait(deadline, [this] { return master_ready_ && slave_ready_; });
  }
  throw std::runtime_error("unknown mode: " +
                           std::to_string(static_cast<int>(mode)));
}

template <typename Pred>
bool SentinelImpl::ConnectedStatus::Wait(engine::Deadline deadline,
                                         const Pred& pred) {
  std::unique_lock<std::mutex> lock(mutex_);
  return cv_.WaitUntil(lock, deadline, pred);
}

}  // namespace redis

USERVER_NAMESPACE_END
