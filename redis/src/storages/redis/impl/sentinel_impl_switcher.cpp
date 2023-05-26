#include "sentinel_impl_switcher.hpp"

#include <memory>

#include <type_traits>
#include <userver/utils/async.hpp>

#include <storages/redis/impl/cluster_sentinel_impl.hpp>

USERVER_NAMESPACE_BEGIN

namespace redis {
ClusterSentinelImplSwitcher::ClusterSentinelImplSwitcher(
    engine::ev::ThreadControl& sentinel_thread_control,
    const std::shared_ptr<engine::ev::ThreadPool>& redis_thread_pool,
    Sentinel& sentinel, const std::vector<std::string>& shards,
    const std::vector<ConnectionInfo>& conns, std::string shard_group_name,
    const std::string& client_name, const Password& password,
    ConnectionSecurity connection_security, ReadyChangeCallback ready_callback,
    std::unique_ptr<KeyShard>&&, dynamic_config::Source dynamic_config_source,
    ConnectionMode mode)
    : params_{sentinel_thread_control,
              redis_thread_pool,
              sentinel,
              shards,
              conns,
              shard_group_name,
              client_name,
              password,
              connection_security,
              ready_callback,
              std::unique_ptr<KeyShard>(),
              dynamic_config_source,
              mode} {}

ClusterSentinelImplSwitcher::~ClusterSentinelImplSwitcher() {
  auto impl = impl_.Get();
  if (impl) {
    impl->Stop();
  }
}

std::unordered_map<ServerId, size_t, ServerIdHasher>
ClusterSentinelImplSwitcher::GetAvailableServersWeighted(
    size_t shard_idx, bool with_master,
    const CommandControl& cc /*= {}*/) const {
  auto impl = impl_.Get();
  UASSERT(impl);
  return impl->GetAvailableServersWeighted(shard_idx, with_master, cc);
}

void ClusterSentinelImplSwitcher::WaitConnectedDebug(bool allow_empty_slaves) {
  auto impl = impl_.Get();
  UASSERT(impl);
  return impl->WaitConnectedDebug(allow_empty_slaves);
}

void ClusterSentinelImplSwitcher::WaitConnectedOnce(
    RedisWaitConnected wait_connected) {
  wait_connected_ = wait_connected;
  auto impl = impl_.Get();
  UASSERT(impl);
  impl->WaitConnectedOnce(std::move(wait_connected));
}

void ClusterSentinelImplSwitcher::ForceUpdateHosts() {
  auto impl = impl_.Get();
  UASSERT(impl);
  impl->ForceUpdateHosts();
}

void ClusterSentinelImplSwitcher::AsyncCommand(
    const SentinelCommand& scommand, size_t prev_instance_idx /*= -1*/) {
  auto impl = impl_.Get();
  UASSERT(impl);
  impl->AsyncCommand(scommand, prev_instance_idx);
}

void ClusterSentinelImplSwitcher::AsyncCommandToSentinel(CommandPtr command) {
  auto impl = impl_.Get();
  UASSERT(impl);
  impl->AsyncCommandToSentinel(std::move(command));
}

size_t ClusterSentinelImplSwitcher::ShardByKey(const std::string& key) const {
  auto impl = impl_.Get();
  UASSERT(impl);
  return impl->ShardByKey(key);
}

size_t ClusterSentinelImplSwitcher::ShardsCount() const {
  auto impl = impl_.Get();
  UASSERT(impl);
  return impl->ShardsCount();
}

const std::string& ClusterSentinelImplSwitcher::GetAnyKeyForShard(
    size_t shard_idx) const {
  auto impl = impl_.Get();
  UASSERT(impl);
  return impl->GetAnyKeyForShard(shard_idx);
}

SentinelStatistics ClusterSentinelImplSwitcher::GetStatistics(
    const MetricsSettings& settings) const {
  auto impl = impl_.Get();
  UASSERT(impl);
  return impl->GetStatistics(settings);
}

void ClusterSentinelImplSwitcher::Init() {
  auto impl = impl_.Get();
  UASSERT(impl);
  return impl->Init();
}

void ClusterSentinelImplSwitcher::Start() {
  auto impl = impl_.Get();
  UASSERT(impl);
  return impl->Start();
}

void ClusterSentinelImplSwitcher::Stop() {
  auto impl = impl_.Get();
  UASSERT(impl);
  return impl->Stop();
}

std::vector<std::shared_ptr<const Shard>>
ClusterSentinelImplSwitcher::GetMasterShards() const {
  auto impl = impl_.Get();
  UASSERT(impl);
  return impl->GetMasterShards();
}

bool ClusterSentinelImplSwitcher::IsInClusterMode() const {
  auto impl = impl_.Get();
  UASSERT(impl);
  return impl->IsInClusterMode();
}

void ClusterSentinelImplSwitcher::SetCommandsBufferingSettings(
    CommandsBufferingSettings commands_buffering_settings) {
  auto impl = impl_.Get();
  UASSERT(impl);
  impl->SetCommandsBufferingSettings(std::move(commands_buffering_settings));
}

void ClusterSentinelImplSwitcher::SetReplicationMonitoringSettings(
    const ReplicationMonitoringSettings& replication_monitoring_settings) {
  auto impl = impl_.Get();
  UASSERT(impl);
  impl->SetReplicationMonitoringSettings(replication_monitoring_settings);
}

void ClusterSentinelImplSwitcher::SetClusterAutoTopology(bool auto_topology) {
  enabled_by_config_ = auto_topology;
  UpdateImpl(true, true);
}

void ClusterSentinelImplSwitcher::SetEnabledByConfig(bool auto_topology) {
  enabled_by_config_ = auto_topology;
}

void ClusterSentinelImplSwitcher::UpdateImpl(bool async, bool wait) {
  auto create_cluster_sentinel = [this, wait] {
    auto sentinel = std::make_shared<ClusterSentinelImpl>(
        params_.sentinel_thread_control, params_.redis_thread_pool,
        params_.sentinel, params_.shards, params_.conns,
        params_.shard_group_name, params_.client_name, params_.password,
        params_.connection_security, params_.ready_callback,
        std::unique_ptr<KeyShard>(), params_.dynamic_config_source,
        params_.mode);
    /// Wait using same settings that were requested by client
    if (wait) {
      params_.sentinel_thread_control.RunInEvLoopBlocking(
          [&sentinel] { sentinel->Start(); });
      sentinel->WaitConnectedOnce(wait_connected_);
    }
    if (enabled_by_config_ &&
        dynamic_cast<ClusterSentinelImpl*>(impl_.Get().get()) == nullptr) {
      impl_.Set(std::move(sentinel));
    }
    creating_impl_.store(false);
  };
  auto create_sentinel = [this, wait] {
    auto sentinel = std::make_shared<SentinelImpl>(
        params_.sentinel_thread_control, params_.redis_thread_pool,
        params_.sentinel, params_.shards, params_.conns,
        params_.shard_group_name, params_.client_name, params_.password,
        params_.connection_security, params_.ready_callback,
        std::unique_ptr<KeyShard>(), params_.dynamic_config_source,
        params_.mode);
    /// Wait using same settings that were requested by client
    if (wait) {
      params_.sentinel_thread_control.RunInEvLoopBlocking(
          [&sentinel] { sentinel->Start(); });
      sentinel->WaitConnectedOnce(wait_connected_);
    }
    if (!enabled_by_config_ &&
        dynamic_cast<SentinelImpl*>(impl_.Get().get()) == nullptr) {
      impl_.Set(std::move(sentinel));
    }
    creating_impl_.store(false);
  };

  if (creating_impl_.load()) {
    return;
  }

  if (enabled_by_config_ && !IsAutoTopologySentinel()) {
    creating_impl_.store(true);
    if (!async) {
      create_cluster_sentinel();
    } else {
      create_task_ =
          utils::CriticalAsync("create_sentinel_impl", create_cluster_sentinel);
    }
  } else if (!enabled_by_config_ && !IsUniversalSentinel()) {
    creating_impl_.store(true);
    if (!async) {
      create_sentinel();
    } else {
      create_task_ =
          utils::CriticalAsync("create_sentinel_impl", create_sentinel);
    }
  }
}

bool ClusterSentinelImplSwitcher::IsAutoTopologySentinel() const {
  return dynamic_cast<ClusterSentinelImpl*>(impl_.Get().get()) != nullptr;
}

bool ClusterSentinelImplSwitcher::IsUniversalSentinel() const {
  return dynamic_cast<SentinelImpl*>(impl_.Get().get()) != nullptr;
}

}  // namespace redis

USERVER_NAMESPACE_END
