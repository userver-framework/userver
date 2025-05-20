#include <storages/odbc/detail/cluster_impl.hpp>
#include <storages/odbc/detail/connection.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::detail {

ClusterImpl::ClusterImpl(const std::vector<std::string>& dsns) : dsns_(dsns) {
    for (const auto& dsn : dsns) {
        connections_.push_back(std::make_unique<Connection>(dsn));
    }
}

ResultSet ClusterImpl::Execute([[maybe_unused]] ClusterHostTypeFlags flags, const Query& query) {
    return connections_[0]->Query(query.Statement());
}

}  // namespace storages::odbc::detail

USERVER_NAMESPACE_END