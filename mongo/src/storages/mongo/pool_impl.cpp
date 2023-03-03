#include <storages/mongo/pool_impl.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo::impl {

PoolImpl::PoolImpl(std::string&& id, const PoolConfig& static_config,
                   dynamic_config::Source config_source)
    : id_(std::move(id)),
      stats_verbosity_(static_config.stats_verbosity),
      config_source_(config_source) {}

const std::string& PoolImpl::Id() const { return id_; }

const stats::PoolStatistics& PoolImpl::GetStatistics() const {
  return statistics_;
}

stats::PoolStatistics& PoolImpl::GetStatistics() { return statistics_; }

dynamic_config::Snapshot PoolImpl::GetConfig() const {
  return config_source_.GetSnapshot();
}

StatsVerbosity PoolImpl::GetStatsVerbosity() const { return stats_verbosity_; }

}  // namespace storages::mongo::impl

USERVER_NAMESPACE_END
