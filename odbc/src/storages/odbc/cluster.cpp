#include <userver/storages/odbc/cluster.hpp>

#include <storages/odbc/detail/cluster_impl.hpp>
#include <storages/odbc/odbc_config.hpp>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc {

Cluster::Cluster(const settings::ODBCClusterSettings& settings, clients::dns::Resolver* resolver)
    : impl_(std::make_unique<detail::ClusterImpl>(settings, resolver))
{
    UASSERT(!settings.pools.empty());
}

Cluster::~Cluster() = default;

ResultSet Cluster::Execute(ClusterHostTypeFlags flags, const Query& query) { return impl_->Execute(flags, query); }

ResultSet Cluster::Execute(engine::Deadline deadline, ClusterHostTypeFlags flags, const Query& query) {
    return impl_->Execute(deadline, flags, query);
}

Transaction Cluster::Begin(ClusterHostTypeFlags flags) { return impl_->Begin(flags); }

Transaction Cluster::Begin(engine::Deadline deadline, ClusterHostTypeFlags flags) {
    return impl_->Begin(deadline, flags);
}

void Cluster::WriteStatistics(utils::statistics::Writer& writer) const { impl_->WriteStatistics(writer); }

void Cluster::SetDefaultCommandControl(const CommandControl& cc) { impl_->SetDefaultCommandControl(cc); }

std::optional<std::chrono::milliseconds> Cluster::GetDefaultNetworkTimeout() const {
    return impl_->GetDefaultNetworkTimeout();
}

std::optional<std::chrono::milliseconds> Cluster::GetDefaultStatementTimeout() const {
    return impl_->GetDefaultStatementTimeout();
}

}  // namespace storages::odbc

USERVER_NAMESPACE_END
