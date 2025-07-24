#include <storages/odbc/detail/cluster_impl.hpp>

#include <storages/odbc/detail/pool.hpp>
#include <userver/storages/odbc/cluster_types.hpp>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::detail {

ClusterImpl::ClusterImpl(const settings::ODBCClusterSettings& settings) {
    for (const auto& host : settings.pools) {
        pools_.push_back(std::make_shared<Pool>(host.dsn, host.pool.minSize, host.pool.maxSize));
    }
}

ResultSet ClusterImpl::Execute([[maybe_unused]] ClusterHostTypeFlags flags, const Query& query) {
    if (flags & ClusterHostType::kMaster || flags & ClusterHostType::kNone || pools_.size() == 1) {
        auto conn = pools_[0]->Acquire();
        return conn->Query(query.GetStatementView());
    }

    UINVARIANT(pools_.size() > 1, "Cluster should have at least 2 connections for ClusterHostType::kSlave");
    auto conn = pools_[1]->Acquire();
    return conn->Query(query.GetStatementView());
}

}  // namespace storages::odbc::detail

USERVER_NAMESPACE_END
