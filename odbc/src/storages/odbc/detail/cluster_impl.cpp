#include <storages/odbc/detail/cluster_impl.hpp>

#include <storages/odbc/detail/pool.hpp>
#include <userver/storages/odbc/cluster_types.hpp>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::detail {
namespace {
std::shared_ptr<Pool> SelectPool(ClusterHostTypeFlags flags) {
    if (flags & ClusterHostType::kMaster || flags & ClusterHostType::kNone || pools_.size() == 1) {
        return pools_[0];
    }
    UINVARIANT(pools_.size() > 1, "Cluster should have at least 2 connections for ClusterHostType::kSlave");
    return pools_[1];
}
}

ClusterImpl::ClusterImpl(const settings::ODBCClusterSettings& settings) {
    UINVARIANT(!setiings.empty(), "Pools count should be positive");
    for (const auto& host : settings.pools) {
        pools_.push_back(std::make_shared<Pool>(host.dsn, host.pool.min_size, host.pool.max_size));
    }
}

ResultSet ClusterImpl::Execute([[maybe_unused]] ClusterHostTypeFlags flags, const Query& query) {
    auto conn = SelectPool(flags)->Acquire();
    return conn->Query(query.GetStatementView());
}

Transaction ClusterImpl::Begin(ClusterHostTypeFlags flags) {
    return Transaction{SelectPool(flags)->Acquire()};
}

}  // namespace storages::odbc::detail

USERVER_NAMESPACE_END
