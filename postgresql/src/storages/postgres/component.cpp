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
#include <userver/logging/log.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/cluster_types.hpp>
#include <userver/storages/postgres/dsn.hpp>
#include <userver/storages/postgres/exceptions.hpp>
#include <userver/storages/postgres/statistics.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/storages/secdist/exceptions.hpp>
#include <userver/testsuite/postgres_control.hpp>
#include <userver/testsuite/tasks.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/enumerate.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {
namespace {

constexpr auto kStatisticsName = "postgresql";

storages::postgres::ConnlimitMode ParseConnlimitMode(const std::string& value) {
  if (value == "manual") return storages::postgres::ConnlimitMode::kManual;
  if (value == "auto") return storages::postgres::ConnlimitMode::kAuto;

  UINVARIANT(false, "Unknown connlimit mode: " + value);
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

  initial_settings_.init_mode = config["sync-start"].As<bool>(true)
                                    ? storages::postgres::InitMode::kSync
                                    : storages::postgres::InitMode::kAsync;
  initial_settings_.db_name = db_name_;
  initial_settings_.connlimit_mode =
      ParseConnlimitMode(config["connlimit_mode"].As<std::string>("auto"));

  initial_settings_.topology_settings.max_replication_lag =
      config["max_replication_lag"].As<std::chrono::milliseconds>(
          kDefaultMaxReplicationLag);

  initial_settings_.pool_settings =
      pg_config.pool_settings.GetOptional(name_).value_or(
          config.As<storages::postgres::PoolSettings>());
  initial_settings_.conn_settings =
      pg_config.connection_settings.GetOptional(name_).value_or(
          config.As<storages::postgres::ConnectionSettings>());
  initial_settings_.conn_settings.pipeline_mode =
      initial_config[storages::postgres::kPipelineModeKey];
  initial_settings_.statement_metrics_settings =
      pg_config.statement_metrics_settings.GetOptional(name_).value_or(
          config.As<storages::postgres::StatementMetricsSettings>());

  const auto task_processor_name =
      config["blocking_task_processor"].As<std::string>();
  auto* bg_task_processor = &context.GetTaskProcessor(task_processor_name);

  error_injection::Settings ei_settings;
  auto ei_settings_opt =
      config["error-injection"].As<std::optional<error_injection::Settings>>();
  if (ei_settings_opt) ei_settings = *ei_settings_opt;

  auto& statistics_storage =
      context.FindComponent<components::StatisticsStorage>().GetStorage();
  statistics_holder_ = statistics_storage.RegisterWriter(
      kStatisticsName, [this](utils::statistics::Writer& writer) {
        return ExtendStatistics(writer);
      });

  // Start all clusters here
  LOG_DEBUG() << "Start " << cluster_desc.size() << " shards for " << db_name_;

  const auto& testsuite_pg_ctl =
      context.FindComponent<components::TestsuiteSupport>()
          .GetPostgresControl();
  auto& testsuite_tasks = testsuite::GetTestsuiteTasks(context);

  auto* resolver = clients::dns::GetResolverPtr(config, context);

  int shard_number = 0;
  for (auto& dsns : cluster_desc) {
    auto cluster = std::make_shared<pg::Cluster>(
        std::move(dsns), resolver, *bg_task_processor, initial_settings_,
        storages::postgres::DefaultCommandControls{
            pg_config.default_command_control,
            pg_config.handlers_command_control,
            pg_config.queries_command_control},
        testsuite_pg_ctl, ei_settings, testsuite_tasks, config_source,
        shard_number++);
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

void Postgres::ExtendStatistics(utils::statistics::Writer& writer) {
  for (const auto& [i, cluster] : utils::enumerate(database_->clusters_)) {
    if (cluster) {
      const auto shard_name = "shard_" + std::to_string(i);
      writer.ValueWithLabels(*cluster->GetStatistics(),
                             {{"postgresql_database", db_name_},
                              {"postgresql_database_shard", shard_name}});
    }
  }
}

void Postgres::OnConfigUpdate(const dynamic_config::Snapshot& cfg) {
  const auto& pg_config = cfg.Get<storages::postgres::Config>();
  const auto pool_settings =
      pg_config.pool_settings.GetOptional(name_).value_or(
          initial_settings_.pool_settings);
  auto connection_settings =
      pg_config.connection_settings.GetOptional(name_).value_or(
          initial_settings_.conn_settings);
  connection_settings.pipeline_mode = cfg[storages::postgres::kPipelineModeKey];
  const auto statement_metrics_settings =
      pg_config.statement_metrics_settings.GetOptional(name_).value_or(
          initial_settings_.statement_metrics_settings);

  for (const auto& cluster : database_->clusters_) {
    cluster->ApplyGlobalCommandControlUpdate(pg_config.default_command_control);
    cluster->SetHandlersCommandControl(pg_config.handlers_command_control);
    cluster->SetQueriesCommandControl(pg_config.queries_command_control);
    cluster->SetPoolSettings(pool_settings);
    cluster->SetConnectionSettings(connection_settings);
    cluster->SetStatementMetricsSettings(statement_metrics_settings);
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
    blocking_task_processor:
        type: string
        description: name of task processor for background blocking operations
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
        defaultDescription: 'async'
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
                type: number
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
    connlimit_mode:
        type: string
        enum:
         - auto
         - manual
        description: how to learn a connection pool size
)");
}

}  // namespace components

USERVER_NAMESPACE_END
