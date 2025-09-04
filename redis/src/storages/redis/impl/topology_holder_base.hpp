#pragma once

#include <storages/redis/impl/cluster_topology.hpp>
#include <storages/redis/impl/redis.hpp>
#include <storages/redis/impl/redis_stats.hpp>
#include <storages/redis/impl/shard.hpp>
#include <userver/storages/redis/wait_connected_mode.hpp>

#include <userver/storages/redis/base.hpp>

#include <userver/rcu/rcu.hpp>
#include <userver/utils/retry_budget.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis::impl {

class TopologyHolderBase {
public:
    using HostPort = std::string;

    virtual void Init() = 0;
    virtual void Start() = 0;
    virtual void Stop() = 0;
    virtual bool WaitReadyOnce(engine::Deadline deadline, WaitConnectedMode mode) = 0;
    virtual rcu::ReadablePtr<ClusterTopology, rcu::BlockingRcuTraits> GetTopology() const = 0;
    virtual void SendUpdateClusterTopology() = 0;
    virtual std::shared_ptr<Redis> GetRedisInstance(const HostPort& host_port) const = 0;
    virtual void GetStatistics(SentinelStatistics& stats, const MetricsSettings& settings) const = 0;

    virtual void SetCommandsBufferingSettings(CommandsBufferingSettings settings) = 0;
    virtual void SetReplicationMonitoringSettings(ReplicationMonitoringSettings settings) = 0;
    virtual void SetRetryBudgetSettings(const utils::RetryBudgetSettings& settings) = 0;
    virtual void SetConnectionInfo(const std::vector<ConnectionInfoInt>& info_array) = 0;

    virtual boost::signals2::signal<void(HostPort, Redis::State)>& GetSignalNodeStateChanged() = 0;
    virtual boost::signals2::signal<void(size_t)>& GetSignalTopologyChanged() = 0;
    virtual void UpdatePassword(const Password& password) = 0;
    virtual Password GetPassword() = 0;

    virtual std::string GetReadinessInfo() const = 0;
    virtual ~TopologyHolderBase() = default;
};

}  // namespace storages::redis::impl

USERVER_NAMESPACE_END
