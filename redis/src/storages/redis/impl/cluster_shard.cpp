#include "cluster_shard.hpp"

#include <limits>
#include <memory>
#include <vector>

#include <userver/utils/assert.hpp>

#include "command_control_impl.hpp"

USERVER_NAMESPACE_BEGIN

namespace redis {

namespace {

bool IsNearestServerPing(const CommandControlImpl& control) {
  switch (control.strategy) {
    case CommandControl::Strategy::kEveryDc:
    case CommandControl::Strategy::kDefault:
      return false;
    case CommandControl::Strategy::kLocalDcConductor:
    case CommandControl::Strategy::kNearestServerPing:
      return true;
  }
  /* never reachable */
  UASSERT(false);
  return false;
}

}  // namespace

ClusterShard& ClusterShard::operator=(const ClusterShard& other) {
  if (&other == this) {
    return *this;
  }
  replicas_ = other.replicas_;
  master_ = other.master_;
  current_ = other.current_.load();
  shard_ = other.shard_;
  return *this;
}

ClusterShard& ClusterShard::operator=(ClusterShard&& other) noexcept {
  if (&other == this) {
    return *this;
  }
  replicas_ = std::move(other.replicas_);
  master_ = std::move(other.master_);
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

  UINVARIANT(false, "Unexpected WaitConnectedMode value");
}

bool ClusterShard::AsyncCommand(CommandPtr command) const {
  const CommandControlImpl cc{command->control};
  const auto read_only = command->read_only;

  if (!read_only || !cc.force_server_id.IsAny()) {
    if (const auto& instance = GetAvailableServer(command->control, read_only);
        instance) {
      return instance->AsyncCommand(command);
    }
    return false;
  }

  const auto current = current_++;
  const auto& available_servers = GetAvailableServers(command->control);
  const auto servers_count = available_servers.size();
  const auto is_nearest_ping_server = IsNearestServerPing(cc);
  const auto is_retry = command->counter != 0;

  const auto masters_count = 1;
  const auto max_attempts = replicas_.size() + masters_count + 1;
  for (size_t attempt = 0; attempt < max_attempts; attempt++) {
    const size_t start_idx =
        GetStartIndex(command->control, attempt, is_nearest_ping_server,
                      command->instance_idx, current, servers_count);

    size_t idx = SentinelImpl::kDefaultPrevInstanceIdx;
    const auto instance =
        GetInstance(available_servers, is_retry, start_idx, attempt,
                    is_nearest_ping_server, cc.best_dc_count, &idx);
    if (!instance) {
      continue;
    }
    command->instance_idx = idx;
    if (instance->AsyncCommand(command)) return true;
  }

  LOG_LIMITED_WARNING() << "No Redis server is ready for shard=" << shard_
                        << " slave=" << command->read_only
                        << " available_servers=" << available_servers.size()
                        << command->GetLogExtra();
  return false;
}

void ClusterShard::GetStatistics(bool master, const MetricsSettings& settings,
                                 ShardStatistics& stats) const {
  auto add_to_stats = [&settings, &stats](const auto& instance) {
    if (!instance) {
      return;
    }
    auto master_host_port = instance->GetServerHost() + ":" +
                            std::to_string(instance->GetServerPort());
    auto it = stats.instances.emplace(std::move(master_host_port),
                                      redis::InstanceStatistics(settings));
    auto& inst_stats = it.first->second;
    inst_stats.Fill(instance->GetStatistics());
    stats.shard_total.Add(inst_stats);
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
}

/// Prioritize first command_control.best_dc_count nearest by ping instances.
/// Leave others to be able to fallback
void ClusterShard::GetNearestServersPing(
    const CommandControl& command_control,
    std::vector<RedisConnectionPtr>& instances) {
  const CommandControlImpl cc{command_control};
  const auto num_instances = std::min(cc.best_dc_count, instances.size());
  if (num_instances == 0 && num_instances == instances.size()) {
    /// We want to leave all instances
    return;
  }
  std::partial_sort(
      instances.begin(), instances.begin() + num_instances, instances.end(),
      [](const RedisConnectionPtr& l, const RedisConnectionPtr& r) {
        UASSERT(r->Get() && l->Get());
        return l->Get()->GetPingLatency() < r->Get()->GetPingLatency();
      });
}

ClusterShard::RedisPtr ClusterShard::GetAvailableServer(
    const CommandControl& command_control, bool read_only) const {
  if (!read_only) {
    if (!master_) {
      return {};
    }
    return master_->Get();
  }

  const CommandControlImpl cc{command_control};
  if (cc.force_server_id.IsAny()) {
    return {};
  }

  if (master_) {
    auto master = master_->Get();
    if (master->GetServerId() == cc.force_server_id) {
      return master;
    }
  }

  for (const auto& replica_connection : replicas_) {
    if (!replica_connection) {
      continue;
    }
    auto replica = replica_connection->Get();
    if (replica->GetServerId() == cc.force_server_id) {
      return replica;
    }
  }
  const logging::LogExtra log_extra(
      {{"server_id", cc.force_server_id.GetId()}});
  LOG_LIMITED_WARNING() << "server_id not found in Redis shard (dead server?)"
                        << log_extra;
  return {};
}

std::vector<ClusterShard::RedisConnectionPtr> ClusterShard::GetAvailableServers(
    const CommandControl& command_control) const {
  const CommandControlImpl cc{command_control};
  if (!IsNearestServerPing(cc)) {
    /// allow_reads_from_master does not matter here.
    /// We just choose right index in GetStartIndex to avoid choosing master
    /// in first try of read_only requests
    return MakeReadonlyWithMasters();
  }

  if (cc.allow_reads_from_master) {
    auto ret = MakeReadonlyWithMasters();
    ClusterShard::GetNearestServersPing(command_control, ret);
    return ret;
  }

  auto ret = replicas_;
  ClusterShard::GetNearestServersPing(command_control, ret);
  ret.push_back(master_);
  return ret;
}

ClusterShard::RedisPtr ClusterShard::GetInstance(
    const std::vector<RedisConnectionPtr>& instances, bool retry,
    size_t start_idx, size_t attempt, bool is_nearest_ping_server,
    size_t best_dc_count, size_t* pinstance_idx) {
  RedisPtr ret;
  const auto end = (is_nearest_ping_server && attempt == 0 && best_dc_count)
                       ? std::min(instances.size(), best_dc_count)
                       : instances.size();
  for (size_t i = 0; i < end; ++i) {
    const auto idx = (start_idx + i) % end;
    const auto& cur = instances[idx];
    if (!cur) {
      continue;
    }
    const auto& cur_inst = cur->Get();

    if (cur_inst && cur_inst->IsAvailable() &&
        (!retry || cur_inst->CanRetry()) &&
        (!ret || ret->IsDestroying() ||
         cur_inst->GetRunningCommands() < ret->GetRunningCommands())) {
      if (pinstance_idx) *pinstance_idx = idx;
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

size_t GetStartIndex(const CommandControl& command_control, size_t attempt,
                     bool is_nearest_ping_server, size_t prev_instance_idx,
                     size_t current, size_t servers_count) {
  const CommandControlImpl cc{command_control};
  size_t best_dc_count = cc.best_dc_count;
  if (best_dc_count == 0) {
    best_dc_count = std::numeric_limits<size_t>::max();
  }
  const bool first_attempt = (attempt == 0);
  const bool first_try =
      prev_instance_idx == SentinelImplBase::kDefaultPrevInstanceIdx;
  /// For compatibility with non-cluster-autotopology driver:
  /// List of available servers for readonly
  /// requests still contains master. Master is the last server in list.
  /// Reads from master are possible even with allow_reads_from_master=false
  /// in cases when there no available replica (replicas are broken or
  /// master is the only instance in cluster shard).
  servers_count = (first_try && first_attempt && !cc.allow_reads_from_master)
                      ? std::max<size_t>(servers_count - 1, 1)
                      : servers_count;
  if (is_nearest_ping_server) {
    /// start index for nearest replicas:
    /// for first try and attempt - just first instance (nearest).
    /// then try other
    return (attempt + (first_try ? (current % std::min<size_t>(best_dc_count,
                                                               servers_count))
                                 : (prev_instance_idx + 1))) %
           servers_count;
  }

  if (first_try) {
    return (current + attempt) % servers_count;
  }

  return (prev_instance_idx + 1 + attempt) % servers_count;
}

std::vector<ClusterShard::RedisConnectionPtr>
ClusterShard::MakeReadonlyWithMasters() const {
  std::vector<RedisConnectionPtr> ret;
  ret.reserve(replicas_.size() + 1);
  ret.insert(ret.end(), replicas_.begin(), replicas_.end());
  ret.push_back(master_);
  return ret;
}

namespace {

ClusterShard::RedisPtr GetRedisIfAvailable(
    const ClusterShard::RedisConnectionPtr& connection,
    const CommandControlImpl& command_control) {
  if (!connection) {
    return {};
  }

  auto ret = connection->Get();

  if (!ret || !ret->IsAvailable() ||
      (!command_control.force_server_id.IsAny() &&
       ret->GetServerId() != command_control.force_server_id)) {
    return {};
  }

  return ret;
}

}  // namespace

ClusterShard::ServersWeighted ClusterShard::GetAvailableServersWeighted(
    bool with_master, const CommandControl& command_control) const {
  ClusterShard::ServersWeighted ret;
  const CommandControlImpl cc{command_control};

  if (with_master && master_) {
    if (const auto redis = GetRedisIfAvailable(master_, cc)) {
      ret.emplace(redis->GetServerId(), 1);
      if (!cc.force_server_id.IsAny() && !ret.empty()) {
        return ret;
      }
    }
  }

  for (const auto& replica_connection : replicas_) {
    if (const auto redis = GetRedisIfAvailable(replica_connection, cc)) {
      ret.emplace(redis->GetServerId(), 1);
      if (!cc.force_server_id.IsAny() && !ret.empty()) {
        return ret;
      }
    }
  }

  return ret;
}

}  // namespace redis

USERVER_NAMESPACE_END
