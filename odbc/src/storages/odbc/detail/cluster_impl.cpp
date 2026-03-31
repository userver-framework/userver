#include <storages/odbc/detail/cluster_impl.hpp>

#include <storages/odbc/detail/deadline.hpp>
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
    return ExecuteImpl(GetExecuteDeadline(kDefaultStatementTimeout), flags, query);
}

ResultSet ClusterImpl::Execute(engine::Deadline deadline, ClusterHostTypeFlags flags, const Query& query) {
    return ExecuteImpl(MergeWithInheritedDeadline(deadline), flags, query);
}

ResultSet ClusterImpl::ExecuteImpl(engine::Deadline effective_deadline, ClusterHostTypeFlags flags, const Query& query) {
    CheckDeadlineNotExpired(effective_deadline);

    tracing::Span span{storages::odbc::impl::tracing::kExecuteSpan};
    auto conn = SelectPool(flags).Acquire(effective_deadline);
    return conn->Query(query.GetStatementView(), effective_deadline);
}

Transaction ClusterImpl::Begin(ClusterHostTypeFlags flags) {
    return BeginImpl(GetExecuteDeadline(kDefaultStatementTimeout), flags);
}

Transaction ClusterImpl::Begin(engine::Deadline deadline, ClusterHostTypeFlags flags) {
    return BeginImpl(MergeWithInheritedDeadline(deadline), flags);
}

Transaction ClusterImpl::BeginImpl(engine::Deadline effective_deadline, ClusterHostTypeFlags flags) {
    CheckDeadlineNotExpired(effective_deadline);

    tracing::Span span{storages::odbc::impl::tracing::kTransactionSpan};
    return Transaction{SelectPool(flags).Acquire(effective_deadline), effective_deadline};
}

Pool& ClusterImpl::SelectPool(ClusterHostTypeFlags flags) const {
    UASSERT(topology_);

    if (flags & ClusterHostType::kSlave) {
        return topology_->SelectPool(ClusterHostType::kSlave);
    }

    // kMaster + kNone go to primary
    return topology_->SelectPool(ClusterHostType::kMaster);
}

void ClusterImpl::WriteStatistics(utils::statistics::Writer& writer) const {
    UASSERT(topology_);
    topology_->WriteStatistics(writer);
}

}  // namespace storages::odbc::detail

USERVER_NAMESPACE_END
