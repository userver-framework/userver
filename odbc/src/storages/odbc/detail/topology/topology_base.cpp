#include <storages/odbc/detail/topology/topology_base.hpp>

#include <storages/odbc/detail/pool.hpp>
#include <storages/odbc/detail/topology/fixed_primary.hpp>
#include <storages/odbc/detail/topology/standalone.hpp>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::detail::topology {

TopologyBase::TopologyBase(const settings::ODBCClusterSettings& settings) {
    UASSERT(!settings.pools.empty());

    pools_.reserve(settings.pools.size());
    for (const auto& host : settings.pools) {
        pools_.push_back(std::make_shared<Pool>(host.dsn, host.pool.max_size, host.pool.max_size));
    }
}

TopologyBase::~TopologyBase() = default;

std::unique_ptr<TopologyBase> TopologyBase::Create(const settings::ODBCClusterSettings& settings) {
    UASSERT(!settings.pools.empty());

    if (settings.pools.size() == 1) {
        return std::make_unique<Standalone>(settings);
    }

    return std::make_unique<FixedPrimary>(settings);
}

Pool& TopologyBase::SelectPool(ClusterHostType host_type) const {
    switch (host_type) {
        case ClusterHostType::kMaster:
        case ClusterHostType::kNone:
            return GetPrimary();
        case ClusterHostType::kSlave:
            return GetSecondary();
    }

    UINVARIANT(false, "Unknown host type");
}

}  // namespace storages::odbc::detail::topology

USERVER_NAMESPACE_END

