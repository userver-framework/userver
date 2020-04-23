#include <storages/postgres/detail/cluster_impl.hpp>

#include <engine/async.hpp>

#include <storages/postgres/dsn.hpp>
#include <storages/postgres/exceptions.hpp>

namespace storages::postgres::detail {

namespace {

struct TryLockGuard {
  TryLockGuard(std::atomic_flag& lock) : lock_(lock) {
    lock_acquired_ = !lock_.test_and_set(std::memory_order_acq_rel);
  }

  ~TryLockGuard() {
    if (lock_acquired_) {
      lock_.clear(std::memory_order_release);
    }
  }

  bool LockAcquired() const { return lock_acquired_; }

 private:
  std::atomic_flag& lock_;
  bool lock_acquired_;
};

}  // namespace

ClusterImpl::ClusterImpl(DsnList dsns, engine::TaskProcessor& bg_task_processor,
                         const PoolSettings& pool_settings,
                         const ConnectionSettings& conn_settings,
                         const CommandControl& default_cmd_ctl,
                         const testsuite::PostgresControl& testsuite_pg_ctl,
                         const error_injection::Settings& ei_settings)
    : topology_(bg_task_processor, std::move(dsns), conn_settings,
                default_cmd_ctl, testsuite_pg_ctl, ei_settings),
      bg_task_processor_(bg_task_processor),
      rr_host_idx_(0) {
  const auto& dsn_list = topology_.GetDsnList();
  if (dsn_list.empty()) {
    throw ClusterError("Cannot create a cluster from an empty DSN list");
  }

  LOG_DEBUG() << "Starting pools initialization";
  host_pools_.reserve(dsn_list.size());
  for (const auto& dsn : dsn_list) {
    host_pools_.push_back(ConnectionPool::Create(
        dsn, bg_task_processor_, pool_settings, conn_settings, default_cmd_ctl,
        testsuite_pg_ctl, ei_settings));
  }
  LOG_DEBUG() << "Pools initialized";
}

ClusterImpl::~ClusterImpl() = default;

ClusterStatisticsPtr ClusterImpl::GetStatistics() const {
  auto cluster_stats = std::make_unique<ClusterStatistics>();
  const auto& dsns = topology_.GetDsnList();
  std::vector<int8_t> is_host_pool_seen(dsns.size(), 0);
  auto dsn_indices_by_type = topology_.GetDsnIndicesByType();

  UASSERT(host_pools_.size() == dsns.size());

  // TODO remove code duplication
  auto master_dsn_indices_it =
      dsn_indices_by_type->find(ClusterHostType::kMaster);
  if (master_dsn_indices_it != dsn_indices_by_type->end() &&
      !master_dsn_indices_it->second.empty()) {
    auto dsn_index = master_dsn_indices_it->second.front();
    UASSERT(dsn_index < dsns.size());
    cluster_stats->master.host_port = GetHostPort(dsns[dsn_index]);
    const auto& pool = host_pools_[dsn_index];
    cluster_stats->master.stats = pool->GetStatistics();
    is_host_pool_seen[dsn_index] = 1;
  }

  auto sync_slave_dsn_indices_it =
      dsn_indices_by_type->find(ClusterHostType::kSyncSlave);
  if (sync_slave_dsn_indices_it != dsn_indices_by_type->end() &&
      !sync_slave_dsn_indices_it->second.empty()) {
    auto dsn_index = sync_slave_dsn_indices_it->second.front();
    UASSERT(dsn_index < dsns.size());
    cluster_stats->sync_slave.host_port = GetHostPort(dsns[dsn_index]);
    UASSERT(dsn_index < host_pools_.size());
    const auto& pool = host_pools_[dsn_index];
    cluster_stats->sync_slave.stats = pool->GetStatistics();
    is_host_pool_seen[dsn_index] = 1;
  }

  auto slaves_dsn_indices_it =
      dsn_indices_by_type->find(ClusterHostType::kSlave);
  if (slaves_dsn_indices_it != dsn_indices_by_type->end() &&
      !slaves_dsn_indices_it->second.empty()) {
    cluster_stats->slaves.reserve(slaves_dsn_indices_it->second.size());
    for (auto dsn_index : slaves_dsn_indices_it->second) {
      if (is_host_pool_seen[dsn_index]) continue;

      InstanceStatsDescriptor slave_desc;
      UASSERT(dsn_index < dsns.size());
      slave_desc.host_port = GetHostPort(dsns[dsn_index]);
      UASSERT(dsn_index < host_pools_.size());
      const auto& pool = host_pools_[dsn_index];
      slave_desc.stats = pool->GetStatistics();
      is_host_pool_seen[dsn_index] = 1;

      cluster_stats->slaves.push_back(std::move(slave_desc));
    }
  }
  for (size_t i = 0; i < is_host_pool_seen.size(); ++i) {
    if (is_host_pool_seen[i]) continue;

    InstanceStatsDescriptor desc;
    UASSERT(i < dsns.size());
    desc.host_port = GetHostPort(dsns[i]);
    UASSERT(i < host_pools_.size());
    const auto& pool = host_pools_[i];
    desc.stats = pool->GetStatistics();

    cluster_stats->unknown.push_back(std::move(desc));
  }
  return cluster_stats;
}

ClusterImpl::ConnectionPoolPtr ClusterImpl::FindPool(ClusterHostType ht) {
  auto host_type = ht;
  auto dsn_indices_by_type = topology_.GetDsnIndicesByType();
  auto dsn_indices_it = dsn_indices_by_type->find(host_type);
  while (host_type != ClusterHostType::kMaster &&
         (dsn_indices_it == dsn_indices_by_type->end() ||
          dsn_indices_it->second.empty())) {
    auto fb = Fallback(host_type);
    LOG_WARNING() << "There is no pool for host type " << ToString(host_type)
                  << ", falling back to " << ToString(fb);
    host_type = fb;
    dsn_indices_it = dsn_indices_by_type->find(host_type);
  }
  if (dsn_indices_it == dsn_indices_by_type->end() ||
      dsn_indices_it->second.empty()) {
    LOG_WARNING() << "Pool for host type (requested: " << ToString(ht)
                  << ", picked " << ToString(host_type) << ") is not available";
    throw ClusterUnavailable("Pool for host type (passed: " + ToString(ht) +
                             ", picked: " + ToString(host_type) +
                             ") is not available");
  }

  auto dsn_index =
      dsn_indices_it
          ->second[rr_host_idx_.fetch_add(1, std::memory_order_relaxed) %
                   dsn_indices_it->second.size()];
  LOG_TRACE() << "Starting transaction on the host of " << host_type << " type";

  UASSERT(dsn_index < host_pools_.size());
  return host_pools_.at(dsn_index);
}

Transaction ClusterImpl::Begin(ClusterHostType ht,
                               const TransactionOptions& options,
                               OptionalCommandControl cmd_ctl) {
  LOG_TRACE() << "Requested transaction on the host of " << ht << " type";
  auto host_type = ht;
  if (options.IsReadOnly()) {
    if (host_type == ClusterHostType::kAny) {
      host_type = ClusterHostType::kSlave;
    }
  } else {
    if (host_type == ClusterHostType::kAny) {
      host_type = ClusterHostType::kMaster;
    } else if (host_type != ClusterHostType::kMaster) {
      throw ClusterUnavailable("Cannot start RW-transaction on a slave");
    }
  }

  return FindPool(host_type)->Begin(options, cmd_ctl);
}

NonTransaction ClusterImpl::Start(ClusterHostType host_type,
                                  OptionalCommandControl cmd_ctl) {
  if (host_type == ClusterHostType::kAny) {
    throw LogicError("Cannot use any host for execution of a single statement");
  }
  LOG_TRACE() << "Requested single statement on the host of " << host_type
              << " type";
  return FindPool(host_type)->Start(cmd_ctl);
}

void ClusterImpl::SetDefaultCommandControl(CommandControl cmd_ctl,
                                           DefaultCommandControlSource source) {
  for (const auto& pool_ptr : host_pools_) {
    pool_ptr->SetDefaultCommandControl(cmd_ctl, source);
  }
}

CommandControl ClusterImpl::GetDefaultCommandControl() const {
  UASSERT(!host_pools_.empty());
  return host_pools_.front()->GetDefaultCommandControl();
}

}  // namespace storages::postgres::detail
