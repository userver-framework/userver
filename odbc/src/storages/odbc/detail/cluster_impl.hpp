#pragma once

#include <memory>
#include <string>
#include <vector>

#include <userver/storages/odbc/cluster_types.hpp>
#include <userver/storages/odbc/query.hpp>
#include <userver/storages/odbc/result_set.hpp>
#include <userver/storages/odbc/settings.hpp>
#include <userver/storages/odbc/transaction.hpp>

#include <userver/engine/deadline.hpp>

#include <storages/odbc/detail/pool.hpp>
#include <storages/odbc/detail/topology/topology_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::detail {

class ClusterImpl {
public:
    ClusterImpl(const settings::ODBCClusterSettings& settings);

    ~ClusterImpl() = default;

    ResultSet Execute(ClusterHostTypeFlags flags, const Query& query);

    ResultSet Execute(engine::Deadline deadline, ClusterHostTypeFlags flags, const Query& query);

    Transaction Begin(ClusterHostTypeFlags flags);

    Transaction Begin(engine::Deadline deadline, ClusterHostTypeFlags flags);

private:
    Pool& SelectPool(ClusterHostTypeFlags flags) const;

    ResultSet ExecuteImpl(engine::Deadline effective_deadline, ClusterHostTypeFlags flags, const Query& query);

    Transaction BeginImpl(engine::Deadline effective_deadline, ClusterHostTypeFlags flags);

    std::unique_ptr<topology::TopologyBase> topology_;
};
}  // namespace storages::odbc::detail

USERVER_NAMESPACE_END
