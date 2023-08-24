#include <storages/mysql/infra/topology/standalone.hpp>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::infra::topology {

Standalone::Standalone(
    clients::dns::Resolver& resolver,
    const std::vector<settings::PoolSettings>& pools_settings)
    : TopologyBase{resolver, pools_settings},
      pool_{InitializePoolReference()} {}

Standalone::~Standalone() = default;

Pool& Standalone::GetPrimary() const { return pool_; }

Pool& Standalone::GetSecondary() const { return GetPrimary(); }

Pool& Standalone::InitializePoolReference() const {
  UASSERT(pools_.size() == 1);
  return *pools_.front();
}

}  // namespace storages::mysql::infra::topology

USERVER_NAMESPACE_END
