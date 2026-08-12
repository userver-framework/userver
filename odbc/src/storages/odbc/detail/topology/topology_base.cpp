#include <storages/odbc/detail/topology/topology_base.hpp>

#include <userver/clients/dns/resolver.hpp>
#include <userver/utils/assert.hpp>

#include <storages/odbc/detail/pool.hpp>
#include <storages/odbc/detail/topology/fixed_primary.hpp>
#include <storages/odbc/detail/topology/standalone.hpp>
#include <storages/odbc/dsn.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::detail::topology {

namespace {

constexpr std::chrono::seconds kDnsResolveTimeout{5};

std::string ResolveDsn(const std::string& dsn_str, clients::dns::Resolver* resolver) {
    if (!resolver) {
        return dsn_str;
    }
    auto resolved = ResolveDsnHost(Dsn{dsn_str}, *resolver, engine::Deadline::FromDuration(kDnsResolveTimeout));
    return resolved.GetUnderlying();
}

}  // namespace

TopologyBase::TopologyBase(const settings::ODBCClusterSettings& settings, clients::dns::Resolver* resolver) {
    UASSERT(!settings.pools.empty());

    pools_.reserve(settings.pools.size());
    for (const auto& host : settings.pools) {
        auto resolved_dsn = ResolveDsn(host.dsn, resolver);
        pools_.push_back(std::make_shared<Pool>(resolved_dsn, host.pool.min_size, host.pool.max_size));
    }
}

TopologyBase::~TopologyBase() = default;

std::unique_ptr<TopologyBase> TopologyBase::Create(
    const settings::ODBCClusterSettings& settings,
    clients::dns::Resolver* resolver
) {
    UASSERT(!settings.pools.empty());

    if (settings.pools.size() == 1) {
        return std::make_unique<Standalone>(settings, resolver);
    }

    return std::make_unique<FixedPrimary>(settings, resolver);
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

void TopologyBase::WriteStatistics(utils::statistics::Writer& writer) const {
    for (std::size_t i = 0; i < pools_.size(); ++i) {
        writer.ValueWithLabels(pools_[i]->GetStatistics(), {{"odbc_pool", std::to_string(i)}});
    }
}

}  // namespace storages::odbc::detail::topology

USERVER_NAMESPACE_END
