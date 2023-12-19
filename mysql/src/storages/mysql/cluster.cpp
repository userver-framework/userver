#include <userver/storages/mysql/cluster.hpp>

#include <vector>

#include <userver/components/component_config.hpp>
#include <userver/tracing/span.hpp>

#include <userver/storages/mysql/impl/tracing_tags.hpp>

#include <storages/mysql/impl/connection.hpp>
#include <storages/mysql/impl/metadata/native_client_info.hpp>
#include <storages/mysql/infra/pool.hpp>
#include <storages/mysql/settings/settings.hpp>

#include <storages/mysql/infra/topology/topology_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql {

namespace {

constexpr std::chrono::seconds kDefaultStatementTimeout{10};

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
    pools_settings.push_back(
        settings::PoolSettings::Create(config, endpoint_info, settings.auth));
  }

  return infra::topology::TopologyBase::Create(resolver, pools_settings);
}

}  // namespace

Cluster::Cluster(clients::dns::Resolver& resolver,
                 const settings::MysqlSettings& settings,
                 const components::ComponentConfig& config)
    : topology_{CreateTopology(resolver, settings, config)} {
  const auto client_info = impl::metadata::NativeClientInfo::Get();
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

CommandResultSet Cluster::ExecuteCommand(ClusterHostType host_type,
                                         const Query& command) const {
  return ExecuteCommand(std::nullopt, host_type, command);
}

CommandResultSet Cluster::ExecuteCommand(OptionalCommandControl command_control,
                                         ClusterHostType host_type,
                                         const Query& command) const {
  const auto deadline =
      GetDeadline(command_control, GetDefaultCommandControl());

  tracing::Span execute_plain_span{impl::tracing::kQuerySpan};

  auto connection = topology_->SelectPool(host_type).Acquire(deadline);

  return CommandResultSet{
      connection->ExecuteQuery(command.GetStatement(), deadline)};
}

void Cluster::WriteStatistics(utils::statistics::Writer& writer) const {
  topology_->WriteStatistics(writer);
}

CommandControl Cluster::GetDefaultCommandControl() {
  return CommandControl{kDefaultStatementTimeout};
}

StatementResultSet Cluster::DoExecute(
    OptionalCommandControl command_control, ClusterHostType host_type,
    const Query& query, impl::io::ParamsBinderBase& params,
    std::optional<std::size_t> batch_size) const {
  const auto deadline =
      GetDeadline(command_control, GetDefaultCommandControl());

  tracing::Span span{impl::tracing::kExecuteSpan};

  auto connection = topology_->SelectPool(host_type).Acquire(deadline);

  auto fetcher = connection->ExecuteStatement(query.GetStatement(), params,
                                              deadline, batch_size);
  return {std::move(connection), std::move(fetcher), std::move(span)};
}

}  // namespace storages::mysql

USERVER_NAMESPACE_END
