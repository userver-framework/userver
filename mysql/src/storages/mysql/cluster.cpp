#include <userver/storages/mysql/cluster.hpp>

#include <vector>

#include <userver/components/component_config.hpp>
#include <userver/tracing/span.hpp>

#include <storages/mysql/impl/metadata/mysql_client_info.hpp>
#include <storages/mysql/impl/mysql_connection.hpp>
#include <storages/mysql/infra/pool.hpp>
#include <storages/mysql/settings/settings.hpp>

#include <storages/mysql/infra/topology/topology_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql {

namespace {

std::unique_ptr<infra::topology::TopologyBase> CreateTopology(
    clients::dns::Resolver& resolver, const settings::MysqlSettings& settings,
    const components::ComponentConfig& config) {
  std::vector<settings::PoolSettings> pools_settings{};
  pools_settings.reserve(settings.endpoints.size());

  for (const auto& endpoint_info : settings.endpoints) {
    pools_settings.emplace_back(config, endpoint_info, settings.auth);
  }

  return infra::topology::TopologyBase::Create(resolver, pools_settings);
}

}  // namespace

Cluster::Cluster(clients::dns::Resolver& resolver,
                 const settings::MysqlSettings& settings,
                 const components::ComponentConfig& config)
    : topology_{CreateTopology(resolver, settings, config)} {
  const auto client_info = impl::metadata::MySQLClientInfo::Get();
  LOG_INFO() << "MySQL cluster initialized."
             << " Native client version: "
             << client_info.client_version_id.ToString();
}

Cluster::~Cluster() = default;

Transaction Cluster::Begin(ClusterHostType host_type,
                           engine::Deadline deadline) {
  auto connection = topology_->SelectPool(host_type).Acquire(deadline);

  return Transaction{std::move(connection)};
}

void Cluster::ExecuteNoPrepare(ClusterHostType host_type,
                               engine::Deadline deadline,
                               const std::string& command) {
  tracing::Span execute_plain_span{"mysql_execute_plain"};

  auto connection = topology_->SelectPool(host_type).Acquire(deadline);

  connection->ExecutePlain(command, deadline);
}

StatementResultSet Cluster::DoExecute(
    ClusterHostType host_type, const std::string& query,
    impl::io::ParamsBinderBase& params, engine::Deadline deadline,
    std::optional<std::size_t> batch_size) const {
  tracing::Span execute_span{"mysql_execute"};

  auto connection = topology_->SelectPool(host_type).Acquire(deadline);

  auto fetcher =
      connection->ExecuteStatement(query, params, deadline, batch_size);

  return {std::move(connection), std::move(fetcher)};
}

void Cluster::DoInsert(const std::string& insert_query,
                       impl::io::ParamsBinderBase& params,
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
