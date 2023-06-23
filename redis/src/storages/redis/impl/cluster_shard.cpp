#include "cluster_shard.hpp"

USERVER_NAMESPACE_BEGIN

namespace redis {

ClusterShard& ClusterShard::operator=(const ClusterShard& other) {
  if (&other == this) {
    return *this;
  }
  master_ = other.master_;
  replicas_ = other.replicas_;
  current_ = other.current_.load();
  shard_ = other.shard_;
  return *this;
}

ClusterShard& ClusterShard::operator=(ClusterShard&& other) noexcept {
  if (&other == this) {
    return *this;
  }
  master_ = std::move(other.master_);
  replicas_ = std::move(other.replicas_);
  current_ = other.current_.load();
  shard_ = other.shard_;
  return *this;
}

bool ClusterShard::IsReady(WaitConnectedMode mode) const {
  switch (mode) {
    case WaitConnectedMode::kNoWait:
      return true;
    case WaitConnectedMode::kMaster:
      return IsMasterReady();
    case WaitConnectedMode::kMasterOrSlave:
      return IsMasterReady() || IsReplicaReady();
    case WaitConnectedMode::kSlave:
      return IsReplicaReady();
    case WaitConnectedMode::kMasterAndSlave:
      return IsMasterReady() && IsReplicaReady();
  }
}

bool ClusterShard::AsyncCommand(CommandPtr command) {
  const auto read_only = command->read_only;
  const auto with_masters =
      !read_only || command->control.allow_reads_from_master;
  const auto with_replicas = read_only;
  const auto& available_servers = GetAvailableServers(
      master_, replicas_, command->control, with_masters, with_replicas);

  const auto masters_count = 1;
  const auto max_attempts = replicas_.size() + masters_count + 1;
  RedisPtr instance;
  for (size_t attempt = 0; attempt < max_attempts; attempt++) {
    const size_t skip_idx = (attempt == 0) ? command->instance_idx : -1;
    instance = GetInstance(available_servers, skip_idx);
    if (!instance) {
      continue;
    }
    command->instance_idx = ToInstanceIdx(instance);
    if (instance->AsyncCommand(command)) return true;
  }

  LOG_LIMITED_WARNING() << "No Redis server is ready for shard=" << shard_
                        << " slave=" << command->read_only
                        << " available_servers=" << available_servers.size()
                        << command->log_extra;
  return false;
}

ShardStatistics ClusterShard::GetStatistics(
    bool master, const MetricsSettings& settings) const {
  ShardStatistics stats(settings);
  auto add_to_stats = [&settings, &stats](const auto& instance) {
    auto inst_stats =
        redis::InstanceStatistics(settings, instance->GetStatistics());
    stats.shard_total.Add(inst_stats);
    auto master_host_port = instance->GetServerHost() + ":" +
                            std::to_string(instance->GetServerPort());
    stats.instances.emplace(std::move(master_host_port), std::move(inst_stats));
  };

  if (master) {
    if (master_) {
      add_to_stats(master_);
    }
  } else {
    for (const auto& instance : replicas_) {
      if (instance) {
        add_to_stats(instance);
      }
    }
  }

  stats.is_ready = IsReady(WaitConnectedMode::kMasterAndSlave);
  return stats;
}

void ClusterShard::GetNearestServersPing(const CommandControl& command_control,
                                         std::vector<RedisPtr>& instances) {
  const auto num_instances =
      std::min(command_control.best_dc_count, instances.size());
  if (num_instances == 0 && num_instances == instances.size()) {
    /// We want to leave all instances
    return;
  }
  /// leave only 'num_instances' best instances
  std::partial_sort(instances.begin(), instances.begin() + num_instances,
                    instances.end(), [](const RedisPtr& l, const RedisPtr& r) {
                      return l->GetPingLatency() < r->GetPingLatency();
                    });
  instances.resize(num_instances);
}

std::vector<ClusterShard::RedisPtr> ClusterShard::GetAvailableServers(
    const RedisPtr& master, const std::vector<RedisPtr>& replicas,
    const CommandControl& command_control, bool with_masters,
    bool with_slaves) {
  if (!command_control.force_server_id.IsAny()) {
    const auto id = command_control.force_server_id;
    if (master->GetServerId() == id) {
      return {master};
    }
    for (const auto& replica : replicas) {
      if (replica->GetServerId() == id) {
        return {replica};
      }
    }
    const logging::LogExtra log_extra({{"server_id", id.GetId()}});
    LOG_LIMITED_WARNING() << "server_id not found in Redis shard (dead server?)"
                          << log_extra;
    return {};
  }

  std::vector<RedisPtr> result;
  result.reserve(with_masters + with_slaves * replicas.size());
  if (with_masters) {
    result.push_back(master);
  }
  if (with_slaves) {
    result.insert(result.end(), replicas.begin(), replicas.end());
  }

  switch (command_control.strategy) {
    case CommandControl::Strategy::kEveryDc:
    case CommandControl::Strategy::kDefault:
      break;
    case CommandControl::Strategy::kLocalDcConductor:
    case CommandControl::Strategy::kNearestServerPing: {
      GetNearestServersPing(command_control, result);
    } break;
  }

  return result;
}

ClusterShard::RedisPtr ClusterShard::GetInstance(
    const std::vector<RedisPtr>& instances, size_t skip_idx) {
  RedisPtr ret;
  const auto size = instances.size();
  const auto cur = ++current_;
  for (size_t i = 0; i < size; ++i) {
    const size_t instance_idx = (cur + i) % size;
    const auto& cur_inst = instances[instance_idx];
    if (skip_idx == ToInstanceIdx(cur_inst)) {
      continue;
    }

    if (cur_inst && !cur_inst->IsDestroying() &&
        (cur_inst->GetState() == Redis::State::kConnected) &&
        !cur_inst->IsSyncing() &&
        (!ret || ret->IsDestroying() ||
         cur_inst->GetRunningCommands() < ret->GetRunningCommands())) {
      ret = cur_inst;
    }
  }
  return ret;
}

bool ClusterShard::IsMasterReady() const {
  return master_ && master_->GetState() == Redis::State::kConnected;
}

bool ClusterShard::IsReplicaReady() const {
  return std::any_of(
      replicas_.begin(), replicas_.end(), [](const RedisPtr& replica) {
        return replica && replica->GetState() == Redis::State::kConnected;
      });
}
}  // namespace redis

USERVER_NAMESPACE_END
