#include <userver/storages/mysql/cluster.hpp>

#include <userver/tracing/span.hpp>

#include <storages/mysql/impl/mysql_connection.hpp>
#include <storages/mysql/infra/pool.hpp>
#include <storages/mysql/infra/topology.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql {

Cluster::Cluster(std::vector<settings::HostSettings>&& settings)
    : topology_(std::move(settings)) {}

Cluster::~Cluster() = default;

StatementResultSet Cluster::DoExecute(ClusterHostType host_type,
                                      const std::string& query,
                                      io::ParamsBinderBase& params,
                                      engine::Deadline deadline) const {
  tracing::Span execute_span{"mysql_execute"};

  auto connection = topology_->SelectPool(host_type).Acquire(deadline);

  auto fetcher = connection->ExecuteStatement(query, params, deadline);

  return {std::move(connection), std::move(fetcher)};
}

void Cluster::DoInsert(const std::string& insert_query,
                       io::ParamsBinderBase& params,
                       engine::Deadline deadline) const {
  tracing::Span insert_span{"mysql_insert"};

  auto connection =
      topology_->SelectPool(ClusterHostType::kMaster).Acquire(deadline);

  connection->ExecuteInsert(insert_query, params, deadline);
}

std::string Cluster::EscapeString(std::string_view source) const {
  return topology_->SelectPool(ClusterHostType::kMaster)
      .Acquire({})
      ->EscapeString(source);
}

}  // namespace storages::mysql

USERVER_NAMESPACE_END
