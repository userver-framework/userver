#include "shard.hpp"

#include <logging/log.hpp>
#include <utils/assert.hpp>

namespace redis {

Shard::Shard(Options options)
    : shard_name_(std::move(options.shard_name)),
      shard_group_name_(std::move(options.shard_group_name)),
      ready_change_callback_(std::move(options.ready_change_callback)),
      read_only_(options.read_only) {
  for (const auto& conn : options.connection_infos) {
    ConnectionInfoInt new_conn;
    static_cast<ConnectionInfo&>(new_conn) = conn;
    connection_infos_.insert(std::move(new_conn));
  }
}

std::unordered_map<ServerId, size_t, ServerIdHasher>
Shard::GetAvailableServersWeighted(
    const CommandControl& command_control) const {
  std::unordered_map<ServerId, size_t, ServerIdHasher> server_weights;
  boost::shared_lock<boost::shared_mutex> lock(mutex_);
  auto available = GetAvailableServers(command_control);
  for (size_t i = 0; i < instances_.size(); i++) {
    const auto& instance = *instances_[i].instance;
    if (available.at(i) && instance.GetState() == Redis::State::kConnected &&
        !instance.IsDestroying()) {
      server_weights.emplace(instance.GetServerId(), 1);
    }
  }
  return server_weights;
}

bool Shard::IsConnectedToAllServersDebug(bool allow_empty) {
  std::lock_guard<boost::shared_mutex> lock(mutex_);
  for (const auto& inst : instances_)
    if (inst.instance->GetState() != Redis::State::kConnected) return false;
  return allow_empty || !instances_.empty();
}

std::vector<unsigned char> Shard::GetAvailableServers(
    const CommandControl& command_control) const {
  if (!command_control.force_server_id.IsAny()) {
    auto id = command_control.force_server_id;
    std::vector<unsigned char> result(instances_.size(), 0);
    for (size_t i = 0; i < instances_.size(); i++) {
      if (instances_[i].instance->GetServerId() == id) {
        result[i] = 1;
        return result;
      }
    }

    logging::LogExtra log_extra({{"server_id", id.GetId()}});
    LOG_WARNING() << "server_id not found in Redis shard (dead server?)"
                  << log_extra;
    return result;
  }

  switch (command_control.strategy) {
    case CommandControl::Strategy::kEveryDc:
    case CommandControl::Strategy::kDefault:
      return std::vector<unsigned char>(instances_.size(), 1);

    case CommandControl::Strategy::kLocalDcConductor:
    case CommandControl::Strategy::kNearestServerPing:
      return GetNearestServersPing(command_control);
  }

  /* never reachable */
  UASSERT(false);
  return std::vector<unsigned char>();
}

std::vector<unsigned char> Shard::GetNearestServersPing(
    const CommandControl& command_control) const {
  auto count = command_control.best_dc_count;
  if (count == 0) count = instances_.size();

  using PairPingNum = std::pair<size_t, size_t>;
  std::vector<PairPingNum> sorted_by_ping;

  sorted_by_ping.reserve(instances_.size());
  for (size_t i = 0; i < instances_.size(); i++) {
    const auto& cur_inst = instances_[i].instance;
    size_t ping = cur_inst->GetPingLatency().count();
    sorted_by_ping.emplace_back(ping, i);
  }

  std::sort(sorted_by_ping.begin(), sorted_by_ping.end());

  auto result = std::vector<unsigned char>(instances_.size(), 0);
  auto size = std::min(sorted_by_ping.size(), count);
  for (unsigned i = 0; i < size; ++i) {
    int num = sorted_by_ping[i].second;
    result[num] = 1;
    LOG_DEBUG() << "Trying redis server with acceptable ping, server="
                << instances_[num].instance->GetServerHost() << ", ping="
                << instances_[num].instance->GetPingLatency().count();
  }
  return result;
}

std::shared_ptr<Redis> Shard::GetInstance(
    const std::vector<unsigned char>& available_servers,
    bool may_fallback_to_any, size_t skip_idx, size_t* pinstance_idx) {
  std::shared_ptr<Redis> instance;

  auto end = instances_.size();
  size_t cur = ++current_;
  for (size_t i = 0; i < end; i++) {
    size_t instance_idx = (cur + i) % end;
    if (instance_idx == skip_idx) continue;

    if (!may_fallback_to_any && !available_servers[instance_idx]) continue;

    const auto& cur_inst = instances_[instance_idx].instance;
    if (cur_inst && !cur_inst->IsDestroying() &&
        (cur_inst->GetState() == Redis::State::kConnected) &&
        (!instance || instance->IsDestroying() ||
         cur_inst->GetRunningCommands() < instance->GetRunningCommands())) {
      if (pinstance_idx) *pinstance_idx = instance_idx;
      instance = cur_inst;
    }
  }

  // nothing found
  return instance;
}

std::vector<ServerId> Shard::GetAllInstancesServerId() const {
  std::vector<ServerId> ids;
  boost::shared_lock<boost::shared_mutex> lock(mutex_);  // protects instances_

  for (const auto& conn_status : instances_) {
    auto instance = conn_status.instance;
    if (instance && instance->GetState() == Redis::State::kConnected &&
        !instance->IsDestroying())
      ids.push_back(instance->GetServerId());
  }
  return ids;
}

bool Shard::AsyncCommand(CommandPtr command, size_t* pinstance_idx) {
  std::shared_ptr<Redis> instance;
  size_t idx = 0;

  boost::shared_lock<boost::shared_mutex> lock(
      mutex_);  // protects instances_ and destroying_
  if (destroying_) return false;

  const auto& available_servers = GetAvailableServers(command->control);

  auto max_attempts = instances_.size() + 1;
  for (size_t attempt = 0; attempt < max_attempts; attempt++) {
    size_t skip_idx = (attempt == 0) && pinstance_idx ? *pinstance_idx : -1;

    /* If we force specific server, use it, don't fallback to any other server.
     * If we don't force specific server:
     * 1) use best servers at the first attemp;
     * 2) fallback to any alive server if (1) failed.
     */
    bool may_fallback_to_any =
        attempt != 0 && command->control.force_server_id.IsAny();

    instance =
        GetInstance(available_servers, may_fallback_to_any, skip_idx, &idx);
    if (pinstance_idx) *pinstance_idx = idx;

    if (instance) {
      if (idx >= available_servers.size() || !available_servers[idx]) {
        LOG_WARNING() << "Failed to use Redis server according to the "
                         "strategy, falling back to any server";
      }
      if (instance->AsyncCommand(command)) return true;
    }
  }

  LOG_WARNING() << "No Redis server is ready for shard_group="
                << shard_group_name_ << " shard=" << shard_name_
                << " slave=" << read_only_;
  return false;
}

void Shard::Clean() {
  // clear 'instances_' and 'clean_wait_' when mutex_ locked
  // destroy ConnectionStatus objects from them when mutex_ unlocked
  std::vector<ConnectionStatus> local_instances;
  std::vector<ConnectionStatus> local_clean_wait;

  {
    boost::unique_lock<boost::shared_mutex> lock(mutex_);
    destroying_ = true;
    local_instances.swap(instances_);
    local_clean_wait.swap(clean_wait_);
  }
}

bool Shard::ProcessCreation(
    const std::shared_ptr<engine::ev::ThreadPool>& redis_thread_pool) {
  auto need_to_create = GetConnectionInfosToCreate();
  // All methods that modify mutex_-protected fields are called from
  // SentinelImpl's event thread.
  // So if we unlock mutex_ after GetConnectionInfosToCreate() and lock it
  // again in UpdateCleanWaitQueue() these fields will remain unchanged.

  std::vector<ConnectionStatus> add_clean_wait;

  for (const auto& id : need_to_create) {
    ConnectionStatus entry;
    entry.info = id;

    entry.instance = std::make_shared<Redis>(redis_thread_pool, read_only_);
    auto server_id = entry.instance->GetServerId();
    entry.instance->signal_state_change.connect(
        [this, server_id](Redis::State state) {
          signal_instance_state_change_(server_id, state);
        });
    entry.instance->Connect(entry.info);

    add_clean_wait.push_back(std::move(entry));
  }

  return UpdateCleanWaitQueue(std::move(add_clean_wait));
}

bool Shard::ProcessStateUpdate() {
  std::vector<ConnectionStatus> erase_clean_wait;
  bool instances_changed = false;
  bool new_connected;
  {
    boost::unique_lock<boost::shared_mutex> lock(mutex_);
    for (auto info = instances_.begin(); info != instances_.end();) {
      if (info->instance->GetState() != Redis::State::kConnected) {
        clean_wait_.emplace_back(std::move(*info));
        info = instances_.erase(info);
        instances_changed = true;
      } else
        ++info;
    }
    for (auto info = clean_wait_.begin(); info != clean_wait_.end();) {
      switch (info->instance->GetState()) {
        case Redis::State::kConnected:
          instances_.emplace_back(std::move(*info));
          instances_changed = true;
          info = clean_wait_.erase(info);
          last_connected_time_ = std::chrono::steady_clock::now();
          break;
        case Redis::State::kDisconnecting:
        case Redis::State::kDisconnected:
        case Redis::State::kDisconnectError:
        case Redis::State::kInitError:
          erase_clean_wait.emplace_back(std::move(*info));
          info = clean_wait_.erase(info);
          break;
        case Redis::State::kInit:
        default:
          ++info;
          break;
      }
    }
    new_connected = !instances_.empty();

    if (!erase_clean_wait.empty() && last_connected_time_ > last_ready_time_) {
      // we were ready, but have just become not ready
      last_ready_time_ = std::chrono::steady_clock::now();
    }
  }

  erase_clean_wait.clear();

  if (prev_connected_ != new_connected) {
    if (ready_change_callback_) {
      try {
        ready_change_callback_(new_connected);
      } catch (const std::exception& ex) {
        LOG_WARNING() << "exception in ready_change_callback_ " << ex.what();
      }
    }
    prev_connected_ = new_connected;
  }
  return instances_changed;
}

bool Shard::SetConnectionInfo(
    const std::vector<ConnectionInfoInt>& info_array) {
  std::set<ConnectionInfoInt> new_info;
  for (const auto& info_entry : info_array) new_info.insert(info_entry);

  boost::unique_lock<boost::shared_mutex> lock(mutex_);
  if (new_info == connection_infos_) return false;
  std::swap(connection_infos_, new_info);
  return true;
}

ShardStatistics Shard::GetStatistics() const {
  boost::shared_lock<boost::shared_mutex> lock(mutex_);
  ShardStatistics stats;

  for (const auto& instance : instances_) {
    if (instance.instance) {
      stats.instances.emplace(
          instance.info.Fulltext(),
          redis::InstanceStatistics(instance.instance->GetStatistics()));
      if (instance.instance->GetState() == Redis::State::kConnected) {
        stats.is_ready = true;
      }
    }
  }
  stats.last_ready_time = last_ready_time_;

  return stats;
}

size_t Shard::InstancesSize() const {
  boost::shared_lock<boost::shared_mutex> lock(mutex_);
  return instances_.size();
}

const std::string& Shard::ShardName() const { return shard_name_; }

boost::signals2::signal<void(ServerId, Redis::State)>&
Shard::SignalInstanceStateChange() {
  return signal_instance_state_change_;
}

std::set<ConnectionInfoInt> Shard::GetConnectionInfosToCreate() const {
  boost::shared_lock<boost::shared_mutex> lock(mutex_);

  std::set<ConnectionInfoInt> need_to_create = connection_infos_;

  for (const auto& instance : instances_) need_to_create.erase(instance.info);
  for (const auto& instance : clean_wait_) need_to_create.erase(instance.info);

  return need_to_create;
}

bool Shard::UpdateCleanWaitQueue(
    std::vector<ConnectionStatus>&& add_clean_wait) {
  bool instances_changed = false;
  std::vector<ConnectionStatus> erase_instance;

  {
    boost::unique_lock<boost::shared_mutex> lock(mutex_);
    for (auto& instance : add_clean_wait)
      clean_wait_.push_back(std::move(instance));

    for (auto instance_iterator = instances_.begin();
         instance_iterator != instances_.end();) {
      if (connection_infos_.find(instance_iterator->info) ==
          connection_infos_.end()) {
        erase_instance.emplace_back(std::move(*instance_iterator));
        instance_iterator = instances_.erase(instance_iterator);
        instances_changed = true;
      } else
        ++instance_iterator;
    }
  }
  return instances_changed;
}

}  // namespace redis
