#pragma once

#include <userver/storages/odbc/cluster_types.hpp>
#include <userver/storages/odbc/query.hpp>
#include <userver/storages/odbc/result_set.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc {

namespace settings {
struct ODBCClusterSettings {
    /// @param dsns list of DSNs to connect to, first DSN is used as a master
    std::vector<std::string> dsns;
};
}  // namespace settings

namespace detail {

class ClusterImpl;
using ClusterImplPtr = std::unique_ptr<ClusterImpl>;

}  // namespace detail

class Cluster {
public:
    Cluster(const settings::ODBCClusterSettings& settings);

    ~Cluster();

    ResultSet Execute(ClusterHostTypeFlags flags, const Query& query);

private:
    detail::ClusterImplPtr impl_;
};
}  // namespace storages::odbc

USERVER_NAMESPACE_END
