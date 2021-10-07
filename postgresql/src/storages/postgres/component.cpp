#include <userver/storages/postgres/component.hpp>

#include <optional>

#include <userver/components/manager.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/error_injection/settings.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/logging/log.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/storages/secdist/exceptions.hpp>
#include <userver/utils/statistics/metadata.hpp>
#include <userver/utils/statistics/percentile_format_json.hpp>

#include <storages/postgres/default_command_controls.hpp>
#include <storages/postgres/detail/connection.hpp>
#include <storages/postgres/postgres_config.hpp>
#include <storages/postgres/postgres_secdist.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/cluster_types.hpp>
#include <userver/storages/postgres/dsn.hpp>
#include <userver/storages/postgres/exceptions.hpp>
#include <userver/storages/postgres/statistics.hpp>
#include <userver/testsuite/postgres_control.hpp>
#include <userver/testsuite/testsuite_support.hpp>

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
    auto cluster = shards[i];
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
      statistics_storage_(
          context.FindComponent<components::StatisticsStorage>()),
      database_{std::make_shared<storages::postgres::Database>()} {
  ::storages::postgres::LogRegisteredTypesOnce();

  namespace pg = storages::postgres;

  auto config_source = context.FindComponent<TaxiConfig>().GetSource();
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

  storages::postgres::ClusterSettings cluster_settings;

  storages::postgres::TopologySettings& topology_settings =
      cluster_settings.topology_settings;
  topology_settings.max_replication_lag =
      config["max_replication_lag"].As<std::chrono::milliseconds>(
          kDefaultMaxReplicationLag);

  storages::postgres::PoolSettings& pool_settings =
      cluster_settings.pool_settings;
  pool_settings.min_size =
      config["min_pool_size"].As<size_t>(kDefaultMinPoolSize);
  pool_settings.max_size =
      config["max_pool_size"].As<size_t>(kDefaultMaxPoolSize);
  pool_settings.max_queue_size =
      config["max_queue_size"].As<size_t>(kDefaultMaxQueueSize);
  pool_settings.sync_start = config["sync-start"].As<bool>(true);
  pool_settings.db_name = db_name_;

  storages::postgres::TaskDataKeysSettings& task_data_keys_settings =
      cluster_settings.task_data_keys_settings;
  task_data_keys_settings.handlers_cmd_ctl_task_data_path_key =
      config["handlers_cmd_ctl_task_data_path_key"]
          .As<std::optional<std::string>>();
  task_data_keys_settings.handlers_cmd_ctl_task_data_method_key =
      config["handlers_cmd_ctl_task_data_method_key"]
          .As<std::optional<std::string>>();

  storages::postgres::ConnectionSettings& conn_settings =
      cluster_settings.conn_settings;
  conn_settings.prepared_statements =
      config["persistent-prepared-statements"].As<bool>(true)
          ? pg::ConnectionSettings::kCachePreparedStatements
          : pg::ConnectionSettings::kNoPreparedStatements;
  conn_settings.user_types = config["user-types-enabled"].As<bool>(true)
                                 ? pg::ConnectionSettings::kUserTypesEnabled
                                 : pg::ConnectionSettings::kPredefinedTypesOnly;
  conn_settings.max_prepared_cache_size =
      config["max_prepared_cache_size"].As<size_t>(
          conn_settings.max_prepared_cache_size);

  conn_settings.ignore_unused_query_params =
      config["ignore_unused_query_params"].As<bool>(false)
          ? pg::ConnectionSettings::kIgnoreUnused
          : pg::ConnectionSettings::kCheckUnused;

  const auto task_processor_name =
      config["blocking_task_processor"].As<std::string>();
  auto* bg_task_processor = &context.GetTaskProcessor(task_processor_name);

  error_injection::Settings ei_settings;
  auto ei_settings_opt =
      config["error-injection"].As<std::optional<error_injection::Settings>>();
  if (ei_settings_opt) ei_settings = *ei_settings_opt;

  statistics_holder_ = statistics_storage_.GetStorage().RegisterExtender(
      kStatisticsName,
      std::bind(&Postgres::ExtendStatistics, this, std::placeholders::_1));

  // Start all clusters here
  LOG_DEBUG() << "Start " << cluster_desc.size() << " shards for " << db_name_;

  const auto& testsuite_pg_ctl =
      context.FindComponent<components::TestsuiteSupport>()
          .GetPostgresControl();

  for (auto& dsns : cluster_desc) {
    auto cluster = std::make_shared<pg::Cluster>(
        std::move(dsns), *bg_task_processor, cluster_settings,
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

void Postgres::OnConfigUpdate(const taxi_config::Snapshot& cfg) {
  const auto& pg_config = cfg.Get<storages::postgres::Config>();
  for (const auto& cluster : database_->clusters_) {
    cluster->ApplyGlobalCommandControlUpdate(pg_config.default_command_control);
    cluster->SetHandlersCommandControl(pg_config.handlers_command_control);
    cluster->SetQueriesCommandControl(pg_config.queries_command_control);
  }
}

}  // namespace components
