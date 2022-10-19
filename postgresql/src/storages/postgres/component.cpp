#include <userver/storages/postgres/component.hpp>

#include <optional>

#include <storages/postgres/default_command_controls.hpp>
#include <storages/postgres/detail/connection.hpp>
#include <storages/postgres/postgres_config.hpp>
#include <storages/postgres/postgres_secdist.hpp>
#include <userver/clients/dns/resolver_utils.hpp>
#include <userver/components/component.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/dynamic_config/storage/component.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/error_injection/settings.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/logging/log.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/cluster_types.hpp>
#include <userver/storages/postgres/dsn.hpp>
#include <userver/storages/postgres/exceptions.hpp>
#include <userver/storages/postgres/statistics.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/storages/secdist/exceptions.hpp>
#include <userver/testsuite/postgres_control.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/statistics/metadata.hpp>
#include <userver/utils/statistics/percentile_format_json.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {
namespace {

constexpr auto kStatisticsName = "postgresql";

formats::json::ValueBuilder InstanceStatisticsToJson(
    const storages::postgres::InstanceStatisticsNonatomic& stats) {
  formats::json::ValueBuilder instance(formats::json::Type::kObject);

  auto conn = instance["connections"];
  conn["opened"] = stats.connection.open_total;
  conn["closed"] = stats.connection.drop_total;
  conn["active"] = stats.connection.active;
  conn["busy"] = stats.connection.used;
  conn["max"] = stats.connection.maximum;
  conn["waiting"] = stats.connection.waiting;
  conn["max-queue-size"] = stats.connection.max_queue_size;

  auto trx = instance["transactions"];
  trx["total"] = stats.transaction.total;
  trx["committed"] = stats.transaction.commit_total;
  trx["rolled-back"] = stats.transaction.rollback_total;
  trx["no-tran"] = stats.transaction.out_of_trx_total;

  auto timing = trx["timings"];
  timing["full"]["1min"] =
      utils::statistics::PercentileToJson(stats.transaction.total_percentile);
  utils::statistics::SolomonSkip(timing["full"]["1min"]);
  timing["busy"]["1min"] =
      utils::statistics::PercentileToJson(stats.transaction.busy_percentile);
  utils::statistics::SolomonSkip(timing["busy"]["1min"]);
  timing["wait-start"]["1min"] = utils::statistics::PercentileToJson(
      stats.transaction.wait_start_percentile);
  utils::statistics::SolomonSkip(timing["wait-start"]["1min"]);
  timing["wait-end"]["1min"] = utils::statistics::PercentileToJson(
      stats.transaction.wait_end_percentile);
  utils::statistics::SolomonSkip(timing["wait-end"]["1min"]);
  timing["return-to-pool"]["1min"] = utils::statistics::PercentileToJson(
      stats.transaction.return_to_pool_percentile);
  utils::statistics::SolomonSkip(timing["return-to-pool"]["1min"]);
  timing["connect"]["1min"] =
      utils::statistics::PercentileToJson(stats.connection_percentile);
  utils::statistics::SolomonSkip(timing["connect"]["1min"]);
  timing["acquire-connection"]["1min"] =
      utils::statistics::PercentileToJson(stats.acquire_percentile);
  utils::statistics::SolomonSkip(timing["acquire-connection"]["1min"]);

  auto query = instance["queries"];
  query["parsed"] = stats.transaction.parse_total;
  query["portals-bound"] = stats.transaction.portal_bind_total;
  query["executed"] = stats.transaction.execute_total;
  query["replies"] = stats.transaction.reply_total;

  auto errors = instance["errors"];
  utils::statistics::SolomonChildrenAreLabelValues(errors, "postgresql_error");
  errors["query-exec"] = stats.transaction.error_execute_total;
  errors["query-timeout"] = stats.transaction.execute_timeout;
  errors["duplicate-prepared-statement"] =
      stats.transaction.duplicate_prepared_statements;
  errors["connection"] = stats.connection.error_total;
  errors["pool"] = stats.pool_exhaust_errors;
  errors["queue"] = stats.queue_size_errors;
  errors["connection-timeout"] = stats.connection.error_timeout;

  instance["prepared-per-connection"] = stats.connection.prepared_statements;
  instance["roundtrip-time"] = stats.topology.roundtrip_time;
  instance["replication-lag"] = stats.topology.replication_lag;

  if (!stats.statement_timings.empty()) {
    auto timings = instance["statement_timings"];
    utils::statistics::SolomonChildrenAreLabelValues(timings,
                                                     "postgresql_query");
    for (const auto& [name, percentile] : stats.statement_timings) {
      timings[name] = utils::statistics::PercentileToJson(percentile);
    }
  }

  utils::statistics::SolomonLabelValue(instance, "postgresql_instance");
  return instance;
}

void AddInstanceStatistics(
    const storages::postgres::InstanceStatsDescriptor& desc,
    formats::json::ValueBuilder& parent) {
  if (!desc.host_port.empty()) {
    parent[desc.host_port] = InstanceStatisticsToJson(desc.stats);
  }
}

formats::json::ValueBuilder ClusterStatisticsToJson(
    storages::postgres::ClusterStatisticsPtr stats) {
  formats::json::ValueBuilder cluster(formats::json::Type::kObject);
  utils::statistics::SolomonChildrenAreLabelValues(
      cluster, "postgresql_cluster_host_type");
  auto master = cluster["master"];
  master = {formats::json::Type::kObject};
  AddInstanceStatistics(stats->master, master);
  auto sync_slave = cluster["sync_slave"];
  sync_slave = {formats::json::Type::kObject};
  AddInstanceStatistics(stats->sync_slave, sync_slave);
  auto slaves = cluster["slaves"];
  slaves = {formats::json::Type::kObject};
  for (const auto& slave : stats->slaves) {
    AddInstanceStatistics(slave, slaves);
  }
  auto unknown = cluster["unknown"];
  unknown = {formats::json::Type::kObject};
  for (const auto& uho : stats->unknown) {
    AddInstanceStatistics(uho, unknown);
  }
  return cluster;
}

formats::json::ValueBuilder PostgresStatisticsToJson(
    const std::vector<storages::postgres::Cluster*>& shards) {
  formats::json::ValueBuilder result(formats::json::Type::kObject);
  for (size_t i = 0; i < shards.size(); ++i) {
    auto* cluster = shards[i];
    if (cluster) {
      const auto shard_name = "shard_" + std::to_string(i);
      result[shard_name] = ClusterStatisticsToJson(cluster->GetStatistics());
    }
  }
  utils::statistics::SolomonChildrenAreLabelValues(result,
                                                   "postgresql_database_shard");
  return result;
}

}  // namespace

Postgres::Postgres(const ComponentConfig& config,
                   const ComponentContext& context)
    : LoggableComponentBase(config, context),
      name_{config.Name()},
      database_{std::make_shared<storages::postgres::Database>()} {
  storages::postgres::LogRegisteredTypesOnce();

  namespace pg = storages::postgres;

  auto config_source = context.FindComponent<DynamicConfig>().GetSource();
  const auto initial_config = config_source.GetSnapshot();
  const auto& pg_config = initial_config.Get<storages::postgres::Config>();

  const auto dbalias = config["dbalias"].As<std::string>("");

  std::vector<pg::DsnList> cluster_desc;
  if (dbalias.empty()) {
    const pg::Dsn dsn{config["dbconnection"].As<std::string>()};
    const auto options = pg::OptionsFromDsn(dsn);
    db_name_ = options.dbname;
    cluster_desc.push_back(pg::SplitByHost(dsn));
  } else {
    try {
      auto& secdist = context.FindComponent<Secdist>();
      cluster_desc = secdist.Get()
                         .Get<pg::secdist::PostgresSettings>()
                         .GetShardedClusterDescription(dbalias);
      db_name_ = dbalias;
    } catch (const storages::secdist::SecdistError& ex) {
      LOG_ERROR() << "Failed to load Postgres config for dbalias " << dbalias
                  << ": " << ex;
      throw;
    }
  }

  const auto monitoring_dbalias =
      config["monitoring-dbalias"].As<std::string>("");
  if (!monitoring_dbalias.empty()) {
    db_name_ = monitoring_dbalias;
  }

  storages::postgres::ClusterSettings cluster_settings;
  cluster_settings.statement_metrics_settings =
      config.As<storages::postgres::StatementMetricsSettings>();

  cluster_settings.pool_settings =
      config.As<storages::postgres::PoolSettings>();
  cluster_settings.init_mode = config["sync-start"].As<bool>(true)
                                   ? storages::postgres::InitMode::kSync
                                   : storages::postgres::InitMode::kAsync;
  cluster_settings.db_name = db_name_;

  storages::postgres::TopologySettings& topology_settings =
      cluster_settings.topology_settings;
  topology_settings.max_replication_lag =
      config["max_replication_lag"].As<std::chrono::milliseconds>(
          kDefaultMaxReplicationLag);

  cluster_settings.conn_settings =
      config.As<storages::postgres::ConnectionSettings>();

  const auto cancel_task_processor_name =
      config["blocking_cancel_task_processor"].As<std::string>();
  auto* bg_cancel_task_processor =
      &context.GetTaskProcessor(cancel_task_processor_name);

  const auto blocking_task_processor_name =
      config["blocking_task_processor"].As<std::string>();
  auto* bg_work_task_processor =
      &context.GetTaskProcessor(blocking_task_processor_name);

  error_injection::Settings ei_settings;
  auto ei_settings_opt =
      config["error-injection"].As<std::optional<error_injection::Settings>>();
  if (ei_settings_opt) ei_settings = *ei_settings_opt;

  auto& statistics_storage =
      context.FindComponent<components::StatisticsStorage>().GetStorage();
  statistics_holder_ = statistics_storage.RegisterExtender(
      kStatisticsName,
      [this](const utils::statistics::StatisticsRequest& request) {
        return ExtendStatistics(request);
      });

  // Start all clusters here
  LOG_DEBUG() << "Start " << cluster_desc.size() << " shards for " << db_name_;

  const auto& testsuite_pg_ctl =
      context.FindComponent<components::TestsuiteSupport>()
          .GetPostgresControl();

  auto* resolver = clients::dns::GetResolverPtr(config, context);

  for (auto& dsns : cluster_desc) {
    auto cluster = std::make_shared<pg::Cluster>(
        std::move(dsns), resolver, *bg_cancel_task_processor,
        *bg_work_task_processor, cluster_settings,
        storages::postgres::DefaultCommandControls{
            pg_config.default_command_control,
            pg_config.handlers_command_control,
            pg_config.queries_command_control},
        testsuite_pg_ctl, ei_settings);
    database_->clusters_.push_back(cluster);
  }

  config_subscription_ = config_source.UpdateAndListen(
      this, "postgres", &Postgres::OnConfigUpdate);

  LOG_DEBUG() << "Component ready";
}

Postgres::~Postgres() {
  statistics_holder_.Unregister();
  config_subscription_.Unsubscribe();
}

storages::postgres::ClusterPtr Postgres::GetCluster() const {
  return database_->GetCluster();
}

storages::postgres::ClusterPtr Postgres::GetClusterForShard(
    size_t shard) const {
  return database_->GetClusterForShard(shard);
}

size_t Postgres::GetShardCount() const { return database_->GetShardCount(); }

formats::json::Value Postgres::ExtendStatistics(
    const utils::statistics::StatisticsRequest& /*request*/) {
  std::vector<storages::postgres::Cluster*> shards_ready;
  shards_ready.reserve(GetShardCount());
  std::transform(database_->clusters_.begin(), database_->clusters_.end(),
                 std::back_inserter(shards_ready),
                 [](const auto& sh) { return sh.get(); });

  formats::json::ValueBuilder result(formats::json::Type::kObject);
  result[db_name_] = PostgresStatisticsToJson(shards_ready);
  utils::statistics::SolomonLabelValue(result[db_name_], "postgresql_database");
  return result.ExtractValue();
}

void Postgres::OnConfigUpdate(const dynamic_config::Snapshot& cfg) {
  const auto& pg_config = cfg.Get<storages::postgres::Config>();
  const auto pool_settings = pg_config.pool_settings.GetOptional(name_);
  auto connection_settings = pg_config.connection_settings.GetOptional(name_);
  const auto statement_metrics_settings =
      pg_config.statement_metrics_settings.GetOptional(name_);
  const auto pipeline_mode = cfg[storages::postgres::kPipelineModeKey];
  for (const auto& cluster : database_->clusters_) {
    cluster->ApplyGlobalCommandControlUpdate(pg_config.default_command_control);
    cluster->SetHandlersCommandControl(pg_config.handlers_command_control);
    cluster->SetQueriesCommandControl(pg_config.queries_command_control);
    if (pool_settings) cluster->SetPoolSettings(*pool_settings);
    if (connection_settings) {
      connection_settings->pipeline_mode = pipeline_mode;
      cluster->SetConnectionSettings(*connection_settings);
    } else {
      cluster->SetPipelineMode(pipeline_mode);
    }
    if (statement_metrics_settings) {
      cluster->SetStatementMetricsSettings(*statement_metrics_settings);
    }
  }
}

yaml_config::Schema Postgres::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<LoggableComponentBase>(R"(
type: object
description: PosgreSQL client component
additionalProperties: false
properties:
    dbalias:
        type: string
        description: name of the database in secdist config (if available)
    dbconnection:
        type: string
        description: connection DSN string (used if no dbalias specified)
    blocking_cancel_task_processor:
        type: string
        description: name of task processor for background blocking cancel operations
    blocking_task_processor:
        type: string
        description: name of task processor for background blocking connect, poll operations
    max_replication_lag:
        type: string
        description: replication lag limit for usable slaves
        defaultDescription: 60s
    min_pool_size:
        type: integer
        description: number of connections created initially
        defaultDescription: 4
    max_pool_size:
        type: integer
        description: limit of connections count
        defaultDescription: 15
    sync-start:
        type: boolean
        description: perform initial connections synchronously
        defaultDescription: false
    dns_resolver:
        type: string
        description: server hostname resolver type (getaddrinfo or async)
        defaultDescription: 'getaddrinfo'
        enum:
          - getaddrinfo
          - async
    persistent-prepared-statements:
        type: boolean
        description: cache prepared statements or not
        defaultDescription: true
    user-types-enabled:
        type: boolean
        description: disabling will disallow use of user-defined types
        defaultDescription: true
    ignore_unused_query_params:
        type: boolean
        description: disable check for not-NULL query params that are not used in query
        defaultDescription: false
    monitoring-dbalias:
        type: string
        description: name of the database for monitorings
        defaultDescription: calculated from dbalias or dbconnection options
    max_prepared_cache_size:
        type: integer
        description: prepared statements cache size limit
        defaultDescription: 5000
    max_statement_metrics:
        type: integer
        description: limit of exported metrics for named statements
        defaultDescription: 0
    error-injection:
        type: object
        description: error-injection options
        additionalProperties: false
        properties:
            enabled:
                type: boolean
                description: enable error injection
                defaultDescription: false
            probability:
                type: double
                description: thrown exception probability
                defaultDescription: 0
            verdicts:
                type: array
                description: possible injection verdicts
                defaultDescription: empty
                items:
                    type: string
                    description: what error injection hook may decide to do
                    enum:
                      - timeout
                      - error
                      - max-delay
                      - random-delay
    max_queue_size:
        type: integer
        description: maximum number of clients waiting for a connection
        defaultDescription: 200
    pipeline_enabled:
        type: boolean
        description: turns on pipeline connection mode
        defaultDescription: false
    connecting_limit:
        type: integer
        description: limit for concurrent establishing connections number per pool (0 - unlimited)
        defaultDescription: 0
)");
}

}  // namespace components

USERVER_NAMESPACE_END
