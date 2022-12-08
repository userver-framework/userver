#include <userver/storages/mysql/cluster.hpp>

#include <storages/mysql/impl/mysql_connection.hpp>
#include <storages/mysql/infra/pool.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql {

Cluster::Cluster() { pool_.push_back(infra::Pool::Create()); }

Cluster::~Cluster() = default;

void Cluster::DoExecute(const std::string& query, io::ParamsBinderBase& params,
                        io::ExtractorBase& extractor,
                        engine::Deadline deadline) {
  auto connection = pool_.back()->Acquire(deadline);

  connection->ExecuteStatement(query, params, extractor, deadline);
}

}  // namespace storages::mysql

USERVER_NAMESPACE_END
