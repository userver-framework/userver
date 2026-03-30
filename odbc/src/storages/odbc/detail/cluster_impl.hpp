#pragma once

#include <memory>
#include <string>
#include <vector>

#include <userver/storages/odbc/cluster_types.hpp>
#include <userver/storages/odbc/query.hpp>
#include <userver/storages/odbc/result_set.hpp>
#include <userver/storages/odbc/settings.hpp>
#include <userver/storages/odbc/transaction.hpp>

#include <storages/odbc/detail/pool.hpp>
#include <storages/odbc/detail/topology/topology_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::detail {

class ClusterImpl {
public:
    ClusterImpl(const settings::ODBCClusterSettings& settings);

    ~ClusterImpl() = default;

    ResultSet Execute(ClusterHostTypeFlags flags, const Query& query);

    Transaction Begin(ClusterHostTypeFlags flags);

private:
    Pool& SelectPool(ClusterHostTypeFlags flags) const;

    std::unique_ptr<topology::TopologyBase> topology_;
};
}  // namespace storages::odbc::detail

USERVER_NAMESPACE_END
