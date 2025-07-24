#include <userver/storages/odbc/cluster.hpp>

#include <storages/odbc/detail/cluster_impl.hpp>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc {

Cluster::Cluster(const settings::ODBCClusterSettings& settings)
    : impl_(std::make_unique<detail::ClusterImpl>(settings)) {
    UASSERT(settings.pools.size() > 0);
}

Cluster::~Cluster() = default;

ResultSet Cluster::Execute(ClusterHostTypeFlags flags, const Query& query) { return impl_->Execute(flags, query); }

}  // namespace storages::odbc

USERVER_NAMESPACE_END
