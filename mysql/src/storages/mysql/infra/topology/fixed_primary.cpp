#include <storages/mysql/infra/topology/fixed_primary.hpp>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::infra::topology {

namespace {

std::size_t WrappingIncrement(std::atomic<size_t>& value, std::size_t mod) {
  // we don't actually care about order being broken once in 2^64 iterations
  return value.fetch_add(1) % mod;
}

}  // namespace

FixedPrimary::FixedPrimary(clients::dns::Resolver& resolver,
                           const std::vector<settings::PoolSettings>& settings)
    : TopologyBase(resolver, settings),
      primary_{InitializePrimaryPoolReference()},
      secondaries_{InitializeSecondariesVector()} {}

FixedPrimary::~FixedPrimary() = default;

Pool& FixedPrimary::GetPrimary() const { return primary_; }

Pool& FixedPrimary::GetSecondary() const {
  const auto pool_index =
      WrappingIncrement(secondary_index_, secondaries_.size());
  return *secondaries_[pool_index];
}

Pool& FixedPrimary::InitializePrimaryPoolReference() {
  UASSERT(pools_.size() > 1);
  return *pools_.front();
}

std::vector<Pool*> FixedPrimary::InitializeSecondariesVector() {
  UASSERT(pools_.size() > 1);

  std::vector<Pool*> pools;
  pools.reserve(pools_.size() - 1);

  for (std::size_t i = 1; i < pools_.size(); ++i) {
    pools.push_back(&*pools_[i]);
  }

  return pools;
}

}  // namespace storages::mysql::infra::topology

USERVER_NAMESPACE_END
