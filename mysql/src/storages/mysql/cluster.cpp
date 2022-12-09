#include <userver/storages/mysql/cluster.hpp>

#include <storages/mysql/impl/mysql_connection.hpp>
#include <storages/mysql/infra/pool.hpp>
#include <storages/mysql/infra/topology.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql {

Cluster::Cluster() = default;

Cluster::~Cluster() = default;

StatementResultSet Cluster::DoExecute(ClusterHostType host_type,
                                      const std::string& query,
                                      io::ParamsBinderBase& params,
                                      engine::Deadline deadline) {
  auto connection = topology_->SelectPool(host_type).Acquire(deadline);

  auto fetcher = connection->ExecuteStatement(query, params, deadline);

  return {std::move(connection), std::move(fetcher)};
}

void Cluster::DoInsert(const std::string& insert_query,
                       io::ParamsBinderBase& params, std::size_t rows_count,
                       engine::Deadline deadline) {
  auto connection =
      topology_->SelectPool(ClusterHostType::kMaster).Acquire(deadline);

  connection->ExecuteInsert(insert_query, params, rows_count, deadline);
}

}  // namespace storages::mysql

USERVER_NAMESPACE_END
