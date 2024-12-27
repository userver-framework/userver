#include "standalone_impl.hpp"

#include <atomic>

#include <fmt/format.h>
#include <boost/container_hash/hash.hpp>
#include <boost/crc.hpp>

#include <userver/concurrent/variable.hpp>
#include <userver/rcu/rcu.hpp>
#include <userver/storages/redis/exception.hpp>
#include <userver/storages/redis/reply.hpp>
#include <userver/utils/algo.hpp>
#include <userver/utils/datetime/steady_coarse_clock.hpp>
#include <userver/utils/fast_scope_guard.hpp>
#include <userver/utils/text.hpp>
#include <userver/logging/log.hpp>

#include <userver/engine/sleep.hpp>
#include <engine/ev/watcher.hpp>
#include <engine/ev/watcher/async_watcher.hpp>
#include <engine/ev/watcher/periodic_watcher.hpp>
#include <storages/redis/impl/cluster_topology.hpp>

#include <storages/redis/impl/sentinel.hpp>

#include "command_control_impl.hpp"

USERVER_NAMESPACE_BEGIN

namespace storages::redis::impl {

namespace {

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

void InvokeCommand(CommandPtr command, ReplyPtr&& reply) {
  UASSERT(reply);

  if (reply->server_id.IsAny()) {
    reply->server_id = CommandControlImpl{command->control}.force_server_id;
  }
  LOG_DEBUG() << "redis_request( " << CommandSpecialPrinter{command}
              << " ):" << (reply->status == ReplyStatus::kOk ? '+' : '-') << ":"
              << reply->time * 1000.0 << " cc: " << command->control.ToString()
              << command->GetLogExtra();
  ++command->invoke_counter;
  try {
    command->callback(command, reply);
  } catch (const std::exception& ex) {
    UASSERT(!engine::current_task::IsTaskProcessorThread());
    LOG_WARNING() << "exception in command->callback, cmd=" << reply->cmd << " "
                  << ex << command->GetLogExtra();
  } catch (...) {
    UASSERT(!engine::current_task::IsTaskProcessorThread());
    LOG_WARNING() << "exception in command->callback, cmd=" << reply->cmd
                  << command->GetLogExtra();
  }
}

}  // namespace

void StandaloneImpl::ProcessWaitingCommands() {
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
    const CommandControlImpl cc{command->control};
    if (scommand.start + cc.timeout_all < now) {
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

void StandaloneImpl::ProcessWaitingCommandsOnStop() {
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

namespace {

constexpr redis::RedisCreationSettings makeRedisCreationSettings() {
    // Нам нужно без READONLY - второй поле структуры RedisCreationSettings в false
    return redis::RedisCreationSettings{ConnectionSecurity::kNone, false};
  }

}  // namespace


StandaloneImpl::StandaloneImpl(
    const engine::ev::ThreadControl& sentinel_thread_control,
    const std::shared_ptr<engine::ev::ThreadPool>& redis_thread_pool,
    ConnectionInfo conn, std::string shard_group_name,
    const std::string& client_name, const Password& password,
    ConnectionSecurity /*connection_security*/,
    ReadyChangeCallback ready_callback,
    dynamic_config::Source dynamic_config_source, ConnectionMode /*mode*/)
    : ev_thread_(sentinel_thread_control),
      process_waiting_commands_timer_(
          std::make_unique<engine::ev::PeriodicWatcher>(
              ev_thread_, [this] { ProcessWaitingCommands(); },
              kSentinelGetHostsCheckInterval)),
      shard_group_name_(std::move(shard_group_name)),
      conn_(std::move(conn)),
      ready_callback_(std::move(ready_callback)),
      redis_thread_pool_(redis_thread_pool),
      client_name_(client_name),
      password_(password),
      dynamic_config_source_(std::move(dynamic_config_source)),
      connection_holder_(new RedisConnectionHolder(
        ev_thread_, redis_thread_pool_, conn_.host, conn_.port, password_,
        CommandsBufferingSettings{}, ReplicationMonitoringSettings{}, utils::RetryBudgetSettings{}, makeRedisCreationSettings())),
      master_shard_ (kUnknownShard, connection_holder_, {}) {
  // https://github.com/boostorg/signals2/issues/59
  // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDelete)
  Init();
  LOG_DEBUG() << "Created StandaloneImpl, shard_group_name="
              << shard_group_name_;
}

StandaloneImpl::~StandaloneImpl() { Stop(); }

std::unordered_map<ServerId, size_t, ServerIdHasher>
StandaloneImpl::GetAvailableServersWeighted(
    size_t /*shard_idx*/, bool with_master, const CommandControl& command_control) const {

  if(!with_master) {
    return {};
  }

  auto redis_conn = connection_holder_->Get();
  const CommandControlImpl cc{command_control};
  if (!redis_conn || !redis_conn->IsAvailable() ||
      (!cc.force_server_id.IsAny() &&
       redis_conn->GetServerId() != cc.force_server_id)) {
    return {};
  }

  return {
    std::make_pair(redis_conn->GetServerId(), 1)
  };
}

void StandaloneImpl::WaitConnectedDebug(bool /*allow_empty_slaves*/) {
  const RedisWaitConnected wait_connected{WaitConnectedMode::kMasterAndSlave,
                                          false,
                                          kRedisWaitConnectedDefaultTimeout};
  WaitConnectedOnce(wait_connected);
}

void StandaloneImpl::WaitConnectedOnce(RedisWaitConnected wait_connected) {
  LOG_DEBUG() << "WaitConnectedOnce in mode " << static_cast<int>(wait_connected.mode);
  LOG_DEBUG() << "Connection holder state = " << static_cast<int>(connection_holder_->GetState());
  LOG_DEBUG() << "Is shard ready = " << master_shard_.IsReady(wait_connected.mode);

  auto deadline = engine::Deadline::FromDuration(wait_connected.timeout);
  while(!master_shard_.IsReady(wait_connected.mode) &&
        !deadline.IsReached()) {
    engine::SleepFor(std::chrono::milliseconds(1));
  }

  if(!master_shard_.IsReady(wait_connected.mode)) {
    const std::string msg = fmt::format(
        "Failed to init cluster slots for redis, shard_group_name={} in {} "
        "ms, mode={}",
        shard_group_name_, wait_connected.timeout.count(),
        ToString(wait_connected.mode));
    if (wait_connected.throw_on_fail) {
      throw ClientNotConnectedException(msg);
    } else {
      LOG_WARNING() << msg << ", starting with not ready Redis client";    
    }
  }
}

void StandaloneImpl::ForceUpdateHosts() {
  throw std::runtime_error(std::string(__func__) + " Unimplemented yet");
}

void StandaloneImpl::Init() {
  
}

void StandaloneImpl::AsyncCommand(const SentinelCommand& scommand,
                                       size_t prev_instance_idx) {
  if (!AdjustDeadline(scommand, dynamic_config_source_.GetSnapshot())) {
    auto reply = std::make_shared<Reply>("", ReplyData::CreateNil());
    reply->status = ReplyStatus::kTimeoutError;
    InvokeCommand(scommand.command, std::move(reply));
    return;
  }

  const CommandPtr command = scommand.command;
  const size_t shard = scommand.shard;
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
        // TODO - скорее всего такой ошибки не в кластере не может просто быть, потому и обработчик здесь этот не нужен
        const bool error_moved = reply->data.IsErrorMoved();
        if (error_moved) {
          const auto& args = ccommand->args.args;
          LOG_WARNING() << "MOVED" << reply->status_string
                      << " c.instance_idx:" << ccommand->instance_idx
                      << " shard: " << shard
                      << " movedto:" << ParseMovedShard(reply->data.GetError())
                      << " args:" << args;
          // this->topology_holder_->SendUpdateClusterTopology();
        }
        const bool retry_to_master =
            !master && reply->data.IsNil() &&
            command->control.force_retries_to_master_on_nil_reply;
        const bool retry = retry_to_master ||
                           reply->status != ReplyStatus::kOk || error_ask ||
                           error_moved || reply->IsUnusableInstanceError() ||
                           reply->IsReadonlyError();

        LOG_DEBUG() << "Is need to retry?: " << retry;
        std::shared_ptr<Redis> moved_to_instance;
        if (retry) {
          const CommandControlImpl cc{command->control};
          const size_t new_shard = shard;
          size_t retries_left = cc.max_retries - 1;

          const std::chrono::steady_clock::time_point until =
              start + cc.timeout_all;
          if (now < until && retries_left > 0) {
            const auto timeout_all =
                std::chrono::duration_cast<std::chrono::milliseconds>(until -
                                                                      now);
            command->control.timeout_single =
                std::min(cc.timeout_single, timeout_all);
            command->control.timeout_all = timeout_all;
            command->control.max_retries = retries_left;

            auto new_command = PrepareCommand(
                std::move(ccommand->args), command->Callback(),
                command->control, command->counter + 1,
                command->asking || error_ask, 0, error_ask || error_moved);
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

  // Здесь нужен мастер шард - для нас это всё один instance
  if (!master_shard_.AsyncCommand(command_check_errors)) {
    scommand.command->args = std::move(command_check_errors->args);
    AsyncCommandFailed(scommand);
    return;
  }
}

void StandaloneImpl::AsyncCommandToSentinel(CommandPtr /*command*/) {
  throw std::runtime_error(std::string(__func__) + " Unimplemented yet");
}

size_t StandaloneImpl::ShardByKey(const std::string& /*key*/) const {
  // здесь всегда возвращать индекс мастер шарда (мастер ноды), т.к. у нас один instance
  // это id  (индекс) нашего одного единственного шарда
  return kUnknownShard;
}

const std::string& StandaloneImpl::GetAnyKeyForShard(
    size_t /*shard_idx*/) const {
  throw std::runtime_error(
      "GetAnyKeyForShard() is not supported in redis cluster mode");
}

void StandaloneImpl::Start() {
  process_waiting_commands_timer_->Start();
}

void StandaloneImpl::AsyncCommandFailed(const SentinelCommand& scommand) {
  // Run command callbacks from redis thread only.
  // It prevents recursive mutex locking in subscription_storage.
  EnqueueCommand(scommand);
}

void StandaloneImpl::Stop() {
  ev_thread_.RunInEvLoopBlocking([this] {
    process_waiting_commands_timer_->Stop();
    ProcessWaitingCommandsOnStop();
  });
}

std::vector<std::shared_ptr<const Shard>> StandaloneImpl::GetMasterShards()
    const {
  throw std::runtime_error("Unimplemented yet");
  /// just return all Shards
  // return {master_shards_.begin(), master_shards_.end()};
}

bool StandaloneImpl::IsInClusterMode() const { return true; }

void StandaloneImpl::SetCommandsBufferingSettings(
    CommandsBufferingSettings commands_buffering_settings) {
  connection_holder_->SetCommandsBufferingSettings(std::move(commands_buffering_settings));
}

void StandaloneImpl::SetReplicationMonitoringSettings(
    const ReplicationMonitoringSettings& monitoring_settings) {
  connection_holder_->SetReplicationMonitoringSettings(std::move(monitoring_settings));
}

void StandaloneImpl::SetRetryBudgetSettings(
    const utils::RetryBudgetSettings& settings) {
  connection_holder_->SetRetryBudgetSettings(std::move(settings));
}

SentinelStatistics StandaloneImpl::GetStatistics(
    const MetricsSettings& settings) const {
  return {settings, {}};
}

void StandaloneImpl::EnqueueCommand(const SentinelCommand& command) {
  const std::lock_guard<std::mutex> lock(command_mutex_);
  commands_.push_back(command);
}

size_t StandaloneImpl::ShardsCount() const {
  return 1;
}

size_t StandaloneImpl::GetClusterSlotsCalledCounter() {
  return 0;
}

PublishSettings StandaloneImpl::GetPublishSettings() {
  return PublishSettings{kUnknownShard, false,
                         CommandControl::Strategy::kEveryDc};
}

void StandaloneImpl::SetConnectionInfo(const std::vector<ConnectionInfoInt>& info_array) {

}

}  // namespace storages::redis::impl

USERVER_NAMESPACE_END
