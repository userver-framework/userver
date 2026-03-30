#include <storages/odbc/detail/cluster_impl.hpp>

#include <storages/odbc/detail/pool.hpp>
#include <storages/odbc/detail/topology/topology_base.hpp>
#include <userver/storages/odbc/cluster_types.hpp>
#include <userver/storages/odbc/impl/tracing_tags.hpp>

#include <userver/tracing/span.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::detail {

ClusterImpl::ClusterImpl(const settings::ODBCClusterSettings& settings) {
    UINVARIANT(!settings.pools.empty(), "Pools count should be positive");
    topology_ = topology::TopologyBase::Create(settings);
}

ResultSet ClusterImpl::Execute([[maybe_unused]] ClusterHostTypeFlags flags, const Query& query) {
    tracing::Span span{storages::odbc::impl::tracing::kExecuteSpan};
    auto conn = SelectPool(flags).Acquire();
    return conn->Query(query.GetStatementView());
}

Transaction ClusterImpl::Begin(ClusterHostTypeFlags flags) {
    tracing::Span span{storages::odbc::impl::tracing::kTransactionSpan};
    return Transaction{SelectPool(flags).Acquire()};
}

Pool& ClusterImpl::SelectPool(ClusterHostTypeFlags flags) const {
    UASSERT(topology_);

    if (flags & ClusterHostType::kSlave) {
        return topology_->SelectPool(ClusterHostType::kSlave);
    }

    // kMaster + kNone go to primary
    return topology_->SelectPool(ClusterHostType::kMaster);
}

}  // namespace storages::odbc::detail

USERVER_NAMESPACE_END
