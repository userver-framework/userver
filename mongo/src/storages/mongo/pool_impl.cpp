#include <storages/mongo/pool_impl.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo::impl {

PoolImpl::PoolImpl(std::string&& id, Config config)
    : id_(std::move(id)), config_storage_(config) {}

const std::string& PoolImpl::Id() const { return id_; }

const stats::PoolStatistics& PoolImpl::GetStatistics() const {
  return statistics_;
}

stats::PoolStatistics& PoolImpl::GetStatistics() { return statistics_; }

rcu::ReadablePtr<Config> PoolImpl::GetConfig() const {
  return config_storage_.Read();
}

void PoolImpl::SetConfig(Config config) { config_storage_.Assign(config); }

}  // namespace storages::mongo::impl

USERVER_NAMESPACE_END
