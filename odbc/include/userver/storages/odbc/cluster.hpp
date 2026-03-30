#pragma once

#include <userver/storages/odbc/cluster_types.hpp>
#include <userver/storages/odbc/query.hpp>
#include <userver/storages/odbc/result_set.hpp>
#include <userver/storages/odbc/settings.hpp>
#include <userver/storages/odbc/transaction.hpp>

#include <userver/engine/deadline.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc {

namespace detail {

class ClusterImpl;
using ClusterImplPtr = std::unique_ptr<ClusterImpl>;

}  // namespace detail

class Cluster {
public:
    Cluster(const settings::ODBCClusterSettings& settings);

    ~Cluster();

    ResultSet Execute(ClusterHostTypeFlags flags, const Query& query);

    ResultSet Execute(engine::Deadline deadline, ClusterHostTypeFlags flags, const Query& query);

    Transaction Begin(ClusterHostTypeFlags flags);

    Transaction Begin(engine::Deadline deadline, ClusterHostTypeFlags flags);

private:
    detail::ClusterImplPtr impl_;
};
}  // namespace storages::odbc

USERVER_NAMESPACE_END
