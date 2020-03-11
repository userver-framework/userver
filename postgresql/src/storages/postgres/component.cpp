#include <storages/postgres/component.hpp>

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

  utils::statistics::SolomonLabelValue(instance, "postgresql_instance");
  return instance;
}

void AddInstanceStatistics(
    const storages::postgres::InstanceStatsDescriptor& desc,
    formats::json::ValueBuilder& parent) {
  if (desc.dsn.empty()) {
    return;
  }
  const auto options = storages::postgres::OptionsFromDsn(desc.dsn);
  auto hostname = options.host;
  if (!options.port.empty()) {
    hostname += ':' + options.port;
  }
  parent[hostname] = InstanceStatisticsToJson(desc.stats);
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
  for (auto i = 0u; i < shards.size(); ++i) {
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

namespace components {

Postgres::Postgres(const ComponentConfig& config,
                   const ComponentContext& context)
    : LoggableComponentBase(config, context),
      statistics_storage_(
          context.FindComponent<components::StatisticsStorage>()),
      database_{std::make_shared<storages::postgres::Database>()} {
  namespace pg = storages::postgres;
  TaxiConfig& cfg{context.FindComponent<TaxiConfig>()};
  auto cmd_ctl = GetCommandControlConfig(cfg.Get());

  const auto dbalias = config.ParseString("dbalias", {});

  if (dbalias.empty()) {
    const auto dsn_string = config.ParseString("dbconnection");
    const auto options = pg::OptionsFromDsn(dsn_string);
    db_name_ = options.dbname;
    cluster_desc_.push_back(pg::SplitByHost(dsn_string));
  } else {
    try {
      auto& secdist = context.FindComponent<Secdist>();
      cluster_desc_ = secdist.Get()
                          .Get<pg::secdist::PostgresSettings>()
                          .GetShardedClusterDescription(dbalias);
      db_name_ = dbalias;
    } catch (const storages::secdist::SecdistError& ex) {
      LOG_ERROR() << "Failed to load Postgres config for dbalias " << dbalias
                  << ": " << ex;
      throw;
    }
  }

  pool_settings_.min_size =
      config.ParseUint64("min_pool_size", kDefaultMinPoolSize);
  pool_settings_.max_size =
      config.ParseUint64("max_pool_size", kDefaultMaxPoolSize);
  pool_settings_.max_queue_size =
      config.ParseUint64("max_queue_size", kDefaultMaxQueueSize);
  pool_settings_.sync_start = config.ParseBool("sync-start", false);

  bool persistent_prepared_statements =
      config.ParseBool("persistent-prepared-statements", true);
  conn_settings_.prepared_statements =
      persistent_prepared_statements
          ? pg::ConnectionSettings::kCachePreparedStatements
          : pg::ConnectionSettings::kNoPreparedStatements;

  const auto task_processor_name =
      config.ParseString("blocking_task_processor");
  bg_task_processor_ = &context.GetTaskProcessor(task_processor_name);

  error_injection::Settings ei_settings;
  auto ei_settings_opt =
      config.Parse<boost::optional<error_injection::Settings>>(
          "error-injection");
  if (ei_settings_opt) ei_settings = *ei_settings_opt;

  statistics_holder_ = statistics_storage_.GetStorage().RegisterExtender(
      kStatisticsName,
      std::bind(&Postgres::ExtendStatistics, this, std::placeholders::_1));

  // Start all clusters here
  LOG_DEBUG() << "Start " << cluster_desc_.size() << " shards for " << db_name_;

  const auto& testsuite_pg_ctl =
      context.FindComponent<components::TestsuiteSupport>()
          .GetPostgresControl();

  for (const auto& dsns : cluster_desc_) {
    auto cluster = std::make_shared<pg::Cluster>(
        dsns, *bg_task_processor_, pool_settings_, conn_settings_, cmd_ctl,
        testsuite_pg_ctl, ei_settings);
    database_->clusters_.push_back(cluster);
  }

  config_subscription_ =
      cfg.AddListener(this, "postgres", &Postgres::OnConfigUpdate);
  OnConfigUpdate(cfg.Get());

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

storages::postgres::CommandControl Postgres::GetCommandControlConfig(
    const TaxiConfigPtr& cfg) const {
  return cfg->Get<storages::postgres::Config>().default_command_control;
}

void Postgres::OnConfigUpdate(const TaxiConfigPtr& cfg) {
  SetDefaultCommandControl(GetCommandControlConfig(cfg));
}

}  // namespace components
