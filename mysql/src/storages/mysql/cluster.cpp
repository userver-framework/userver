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

constexpr std::chrono::milliseconds kDefaultStatementTimeout{1750};

engine::Deadline GetDeadline(OptionalCommandControl optional_cc,
                             CommandControl default_cc) {
  const auto timeout =
      optional_cc.has_value() ? optional_cc->execute : default_cc.execute;

  return engine::Deadline::FromDuration(timeout);
}

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

Transaction Cluster::Begin(ClusterHostType host_type) const {
  return Cluster::Begin(std::nullopt, host_type);
}

Transaction Cluster::Begin(OptionalCommandControl command_control,
                           ClusterHostType host_type) const {
  const auto deadline =
      GetDeadline(command_control, GetDefaultCommandControl());

  return Transaction{topology_->SelectPool(host_type).Acquire(deadline),
                     deadline};
}

void Cluster::ExecuteCommand(ClusterHostType host_type,
                             const Query& command) const {
  return ExecuteCommand(std::nullopt, host_type, command);
}

void Cluster::ExecuteCommand(OptionalCommandControl command_control,
                             ClusterHostType host_type,
                             const Query& command) const {
  const auto deadline =
      GetDeadline(command_control, GetDefaultCommandControl());

  tracing::Span execute_plain_span{"mysql_execute_command"};

  auto connection = topology_->SelectPool(host_type).Acquire(deadline);

  connection->ExecutePlain(command.GetStatement(), deadline);
}

CommandControl Cluster::GetDefaultCommandControl() const {
  return CommandControl{kDefaultStatementTimeout};
}

StatementResultSet Cluster::DoExecute(
    OptionalCommandControl command_control, ClusterHostType host_type,
    const Query& query, impl::io::ParamsBinderBase& params,
    std::optional<std::size_t> batch_size) const {
  const auto deadline =
      GetDeadline(command_control, GetDefaultCommandControl());

  // TODO : name the span
  tracing::Span span{"name_me"};

  auto connection = topology_->SelectPool(host_type).Acquire(deadline);

  auto fetcher = connection->ExecuteStatement(query.GetStatement(), params,
                                              deadline, batch_size);
  return {std::move(connection), std::move(fetcher)};
}

std::string Cluster::EscapeString(std::string_view source) const {
  return topology_->SelectPool(ClusterHostType::kMaster)
      .Acquire({})
      ->EscapeString(source);
}

}  // namespace storages::mysql

USERVER_NAMESPACE_END
