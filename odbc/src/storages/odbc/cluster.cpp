#include <userver/storages/odbc/cluster.hpp>

#include <storages/odbc/detail/cluster_impl.hpp>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc {

Cluster::Cluster(const settings::ODBCClusterSettings& settings)
    : impl_(std::make_unique<detail::ClusterImpl>(settings))
{
    UASSERT(settings.pools.size() > 0);
}

Cluster::~Cluster() = default;

ResultSet Cluster::Execute(ClusterHostTypeFlags flags, const Query& query) { return impl_->Execute(flags, query); }

ResultSet Cluster::Execute(engine::Deadline deadline, ClusterHostTypeFlags flags, const Query& query) {
    return impl_->Execute(deadline, flags, query);
}

Transaction Cluster::Begin(ClusterHostTypeFlags flags) {
    return impl_->Begin(flags);
}

Transaction Cluster::Begin(engine::Deadline deadline, ClusterHostTypeFlags flags) {
    return impl_->Begin(deadline, flags);
}

}  // namespace storages::odbc

USERVER_NAMESPACE_END
