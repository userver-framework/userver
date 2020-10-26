#include <storages/postgres/component.hpp>

#include <optional>

#include <components/manager.hpp>
#include <engine/task/task_processor.hpp>
#include <engine/task/task_processor_config.hpp>
#include <error_injection/settings.hpp>
#include <formats/json/value_builder.hpp>
#include <logging/log.hpp>
#include <storages/secdist/component.hpp>
#include <storages/secdist/exceptions.hpp>
#include <utils/statistics/metadata.hpp>
#include <utils/statistics/percentile_format_json.hpp>

#include <storages/postgres/cluster.hpp>
#include <storages/postgres/cluster_types.hpp>
#include <storages/postgres/dsn.hpp>
#include <storages/postgres/exceptions.hpp>
#include <storages/postgres/postgres_config.hpp>
#include <storages/postgres/postgres_secdist.hpp>
#include <storages/postgres/statistics.hpp>
#include <testsuite/postgres_control.hpp>
#include <testsuite/testsuite_support.hpp>

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

storages::postgres::CommandControl GetCommandControlConfig(
    const std::shared_ptr<const taxi_config::Config>& cfg) {
  return cfg->Get<storages::postgres::Config>().default_command_control;
}

storages::postgres::CommandControl GetCommandControlConfig(
    const TaxiConfig& cfg) {
  auto conf = cfg.Get();
  return GetCommandControlConfig(conf);
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
  TaxiConfig& cfg{context.FindComponent<TaxiConfig>()};
  auto cmd_ctl = GetCommandControlConfig(cfg);

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

  storages::postgres::TopologySettings topology_settings;
  topology_settings.max_replication_lag =
      config["max_replication_lag"].As<std::chrono::milliseconds>(
          kDefaultMaxReplicationLag);

  storages::postgres::PoolSettings pool_settings;
  pool_settings.min_size =
      config["min_pool_size"].As<size_t>(kDefaultMinPoolSize);
  pool_settings.max_size =
      config["max_pool_size"].As<size_t>(kDefaultMaxPoolSize);
  pool_settings.max_queue_size =
      config["max_queue_size"].As<size_t>(kDefaultMaxQueueSize);
  pool_settings.sync_start = config["sync-start"].As<bool>(true);
  pool_settings.db_name = db_name_;

  storages::postgres::ConnectionSettings conn_settings;
  conn_settings.prepared_statements =
      config["persistent-prepared-statements"].As<bool>(true)
          ? pg::ConnectionSettings::kCachePreparedStatements
          : pg::ConnectionSettings::kNoPreparedStatements;

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
        std::move(dsns), *bg_task_processor, topology_settings, pool_settings,
        conn_settings, cmd_ctl, testsuite_pg_ctl, ei_settings);
    database_->clusters_.push_back(cluster);
  }

  config_subscription_ =
      cfg.UpdateAndListen(this, "postgres", &Postgres::OnConfigUpdate);

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

void Postgres::SetDefaultCommandControl(
    storages::postgres::CommandControl cmd_ctl) {
  for (const auto& cluster : database_->clusters_) {
    cluster->SetDefaultCommandControl(cmd_ctl);
  }
}

void Postgres::OnConfigUpdate(const TaxiConfigPtr& cfg) {
  const auto cmd_ctl = GetCommandControlConfig(cfg);
  for (const auto& cluster : database_->clusters_) {
    cluster->ApplyGlobalCommandControlUpdate(cmd_ctl);
  }
}

}  // namespace components
