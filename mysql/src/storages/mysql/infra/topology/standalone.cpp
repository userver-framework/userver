#include <storages/mysql/infra/topology/standalone.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::infra::topology {

Standalone::Standalone(std::vector<settings::HostSettings>&& hosts_settings)
    : TopologyBase{std::move(hosts_settings)}, pool_{InitializePool()} {}

Standalone::~Standalone() = default;

Pool& Standalone::GetMaster() const { return pool_; }

Pool& Standalone::GetSlave() const { return pool_; }

Pool& Standalone::InitializePool() const {
  UASSERT(pools_.size() == 1);
  return *pools_.front();
}

}  // namespace storages::mysql::infra::topology

USERVER_NAMESPACE_END
