#include <storages/odbc/detail/topology/fixed_primary.hpp>

#include <storages/odbc/detail/pool.hpp>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::detail::topology {

namespace {

std::size_t WrappingIncrement(std::atomic<std::size_t>& value, std::size_t mod) { return value.fetch_add(1) % mod; }

}  // namespace

FixedPrimary::FixedPrimary(const settings::ODBCClusterSettings& settings, clients::dns::Resolver* resolver)
    : TopologyBase(settings, resolver),
      primary_{InitializePrimaryPoolReference()},
      secondaries_{InitializeSecondariesVector()}
{}

FixedPrimary::~FixedPrimary() = default;

Pool& FixedPrimary::GetPrimary() const { return primary_; }

Pool& FixedPrimary::GetSecondary() const {
    const auto pool_index = WrappingIncrement(secondary_index_, secondaries_.size());
    return *secondaries_[pool_index];
}

Pool& FixedPrimary::InitializePrimaryPoolReference() {
    UASSERT(GetPools().size() > 1);
    return *GetPools().front();
}

std::vector<Pool*> FixedPrimary::InitializeSecondariesVector() {
    UASSERT(GetPools().size() > 1);

    std::vector<Pool*> pools;
    pools.reserve(GetPools().size() - 1);
    for (std::size_t i = 1; i < GetPools().size(); ++i) {
        pools.push_back(&*GetPools()[i]);
    }

    return pools;
}

}  // namespace storages::odbc::detail::topology

USERVER_NAMESPACE_END
