#include "cluster_shard.hpp"

#include <memory>
#include <userver/utils/assert.hpp>

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

  UASSERT_MSG(false, "Unexpected WaitConnectedMode value");
}

bool ClusterShard::AsyncCommand(CommandPtr command) {
  const auto read_only = command->read_only;
  const auto with_masters =
      !read_only || command->control.allow_reads_from_master;
  const auto with_replicas = read_only;
  const auto& available_servers =
      GetAvailableServers(master_, replicas_, command->control, with_masters,
                          with_replicas, current_++);

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
    if (!instance) {
      return;
    }
    auto inst_stats =
        redis::InstanceStatistics(settings, instance->GetStatistics());
    stats.shard_total.Add(inst_stats);
    auto master_host_port = instance->GetServerHost() + ":" +
                            std::to_string(instance->GetServerPort());
    stats.instances.emplace(std::move(master_host_port), std::move(inst_stats));
  };

  if (master) {
    if (master_) {
      add_to_stats(master_->Get());
    }
  } else {
    for (const auto& instance : replicas_) {
      if (instance) {
        add_to_stats(instance->Get());
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

std::vector<std::shared_ptr<Redis>> ClusterShard::GetAvailableServers(
    const ClusterShard::RedisConnectionPtr& master_connetion,
    const std::vector<ClusterShard::RedisConnectionPtr>& replicas_connections,
    const CommandControl& command_control, bool with_masters, bool with_slaves,
    size_t current) {
  if (!command_control.force_server_id.IsAny()) {
    const auto id = command_control.force_server_id;
    auto master = master_connetion->Get();
    if (master->GetServerId() == id) {
      return {std::move(master)};
    }
    for (const auto& replica_connection : replicas_connections) {
      auto replica = replica_connection->Get();
      if (replica->GetServerId() == id) {
        return {std::move(replica)};
      }
    }
    const logging::LogExtra log_extra({{"server_id", id.GetId()}});
    LOG_LIMITED_WARNING() << "server_id not found in Redis shard (dead server?)"
                          << log_extra;
    return {};
  }

  std::vector<ClusterShard::RedisPtr> result;
  result.reserve(with_masters + with_slaves * replicas_connections.size());

  /// Define retry priority
  if (with_masters && !with_slaves) {
    /// First attempt will be made in master and then in replicas
    result.push_back(master_connetion->Get());
    const auto replicas_count = replicas_connections.size();
    for (size_t i = 0; i < replicas_count; ++i) {
      const auto index = (current + i) % replicas_count;
      result.push_back(replicas_connections[index]->Get());
    }
  } else {
    /// First attempts will be made in replicas and after that in master
    const auto replicas_count = replicas_connections.size();
    for (size_t i = 0; i < replicas_count; ++i) {
      const auto index = (current + i) % replicas_count;
      result.push_back(replicas_connections[index]->Get());
    }
    result.push_back(master_connetion->Get());
  }

  switch (command_control.strategy) {
    case CommandControl::Strategy::kEveryDc:
    case CommandControl::Strategy::kDefault:
      break;
    case CommandControl::Strategy::kLocalDcConductor:
    case CommandControl::Strategy::kNearestServerPing: {
      ClusterShard::GetNearestServersPing(command_control, result);
    } break;
  }

  return result;
}

ClusterShard::RedisPtr ClusterShard::GetInstance(
    const std::vector<RedisPtr>& instances, size_t skip_idx) {
  RedisPtr ret;
  for (const auto& cur_inst : instances) {
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
      replicas_.begin(), replicas_.end(), [](const auto& replica) {
        return replica && replica->GetState() == Redis::State::kConnected;
      });
}
}  // namespace redis

USERVER_NAMESPACE_END
