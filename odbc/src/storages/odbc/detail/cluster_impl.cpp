#include <storages/odbc/detail/cluster_impl.hpp>

#include <storages/odbc/detail/connection.hpp>
#include <userver/storages/odbc/cluster_types.hpp>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::detail {

ClusterImpl::ClusterImpl(const std::vector<std::string>& dsns) : dsns_(dsns) {
    for (const auto& dsn : dsns) {
        connections_.push_back(std::make_unique<Connection>(dsn));
    }
}

ResultSet ClusterImpl::Execute([[maybe_unused]] ClusterHostTypeFlags flags, const Query& query) {
    if (flags & ClusterHostType::kMaster || flags & ClusterHostType::kNone || connections_.size() == 1) {
        return connections_[0]->Query(query.Statement());
    }

    UINVARIANT(connections_.size() > 1, "Cluster should have at least 2 connections for ClusterHostType::kSlave");
    return connections_[1]->Query(query.Statement());
}

}  // namespace storages::odbc::detail

USERVER_NAMESPACE_END
