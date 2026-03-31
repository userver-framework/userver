#include <storages/odbc/detail/cluster_impl.hpp>

#include <storages/odbc/detail/deadline.hpp>
#include <storages/odbc/detail/pool.hpp>
#include <storages/odbc/detail/topology/topology_base.hpp>
#include <userver/storages/odbc/cluster_types.hpp>
#include <userver/storages/odbc/exception.hpp>
#include <userver/storages/odbc/impl/tracing_tags.hpp>

#include <userver/tracing/span.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/datetime/steady_coarse_clock.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::detail {

ClusterImpl::ClusterImpl(const settings::ODBCClusterSettings& settings, clients::dns::Resolver* resolver) {
    UINVARIANT(!settings.pools.empty(), "Pools count should be positive");
    topology_ = topology::TopologyBase::Create(settings, resolver);
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
    auto& pool = SelectPool(flags);
    auto conn = pool.Acquire(effective_deadline);

    pool.AccountOutOfTransaction();

    const auto start = utils::datetime::SteadyCoarseClock::now();
    try {
        auto result = conn->Query(query.GetStatementView(), effective_deadline);
        const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
            utils::datetime::SteadyCoarseClock::now() - start
        );
        pool.AccountQueryExecuted(elapsed);
        return result;
    } catch (const OperationInterrupted&) {
        pool.AccountQueryTimeout();
        throw;
    } catch (const Error&) {
        pool.AccountQueryError();
        throw;
    }
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
    auto& pool = SelectPool(flags);
    return Transaction{pool.Acquire(effective_deadline), pool, effective_deadline};
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
