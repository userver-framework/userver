#include <storages/postgres/component.hpp>

#include <components/manager.hpp>
#include <engine/task/task_processor.hpp>
#include <engine/task/task_processor_config.hpp>
#include <formats/json/value_builder.hpp>
#include <logging/log.hpp>
#include <storages/secdist/component.hpp>
#include <storages/secdist/exceptions.hpp>
#include <utils/statistics/percentile_format_json.hpp>

#include <storages/postgres/cluster.hpp>
#include <storages/postgres/cluster_types.hpp>
#include <storages/postgres/dsn.hpp>
#include <storages/postgres/exceptions.hpp>
#include <storages/postgres/postgres_config.hpp>
#include <storages/postgres/postgres_secdist.hpp>
#include <storages/postgres/statistics.hpp>

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

  auto trx = instance["transactions"];
  trx["total"] = stats.transaction.total;
  trx["committed"] = stats.transaction.commit_total;
  trx["rolled-back"] = stats.transaction.rollback_total;
  trx["no-tran"] = stats.transaction.out_of_trx_total;

  auto timing = trx["timings"];
  timing["full"]["1min"] =
      utils::statistics::PercentileToJson(stats.transaction.total_percentile);
  timing["busy"]["1min"] =
      utils::statistics::PercentileToJson(stats.transaction.busy_percentile);

  auto query = instance["queries"];
  query["parsed"] = stats.transaction.parse_total;
  query["executed"] = stats.transaction.execute_total;
  query["replies"] = stats.transaction.reply_total;
  query["binary-replies"] = stats.transaction.bin_reply_total;

  auto errors = instance["errors"];
  errors["query-exec"] = stats.transaction.error_execute_total;
  errors["connection"] = stats.connection.error_total;
  errors["pool"] = stats.pool_error_exhaust_total;

  return instance;
}

void AddInstanceStatistics(
    const storages::postgres::InstanceStatsDescriptor& desc,
    formats::json::ValueBuilder& parent) {
  if (desc.dsn.empty()) {
    return;
  }
  const auto options = storages::postgres::OptionsFromDsn(desc.dsn);
  parent[options.host + ':' + options.port] =
      InstanceStatisticsToJson(desc.stats);
}

formats::json::ValueBuilder ClusterStatisticsToJson(
    const storages::postgres::ClusterStatistics& stats) {
  formats::json::ValueBuilder cluster(formats::json::Type::kObject);
  auto master = cluster["master"];
  AddInstanceStatistics(stats.master, master);
  auto sync_slave = cluster["sync_slave"];
  AddInstanceStatistics(stats.sync_slave, sync_slave);
  auto slaves = cluster["slaves"];
  slaves = {formats::json::Type::kObject};
  for (auto i = 0u; i < stats.slaves.size(); ++i) {
    AddInstanceStatistics(stats.slaves[i], slaves);
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
  return result;
}

}  // namespace

namespace components {

Postgres::Postgres(const ComponentConfig& config,
                   const ComponentContext& context)
    : LoggableComponentBase(config, context),
      statistics_storage_(
          context.FindComponent<components::StatisticsStorage>()) {
  TaxiConfig& cfg{context.FindComponent<TaxiConfig>()};
  config_subscription_ =
      cfg.AddListener(this, "postgres", &Postgres::OnConfigUpdate);
  OnConfigUpdate(cfg.Get());
  auto cmd_ctl = GetCommandControlConfig(cfg.Get());

  const auto dbalias = config.ParseString("dbalias", {});

  if (dbalias.empty()) {
    const auto dsn_string = config.ParseString("dbconnection");
    const auto options = storages::postgres::OptionsFromDsn(dsn_string);
    db_name_ = options.dbname;
    cluster_desc_.push_back(storages::postgres::ClusterDescription(
        storages::postgres::SplitByHost(dsn_string)));
  } else {
    try {
      auto& secdist = context.FindComponent<Secdist>();
      cluster_desc_ = secdist.Get()
                          .Get<storages::postgres::secdist::PostgresSettings>()
                          .GetShardedClusterDescription(dbalias);
      db_name_ = dbalias;
    } catch (const storages::secdist::SecdistError& ex) {
      LOG_ERROR() << "Failed to load Postgres config for dbalias " << dbalias
                  << ": " << ex.what();
      throw;
    }
  }

  min_pool_size_ = config.ParseUint64("min_pool_size", kDefaultMinPoolSize);
  max_pool_size_ = config.ParseUint64("max_pool_size", kDefaultMaxPoolSize);

  const auto task_processor_name =
      config.ParseString("blocking_task_processor");
  bg_task_processor_ = &context.GetTaskProcessor(task_processor_name);

  statistics_holder_ = statistics_storage_.GetStorage().RegisterExtender(
      kStatisticsName,
      std::bind(&Postgres::ExtendStatistics, this, std::placeholders::_1));

  // Start all clusters here
  LOG_DEBUG() << "Start " << cluster_desc_.size() << " shards for " << db_name_;
  std::vector<engine::TaskWithResult<void>> tasks;
  for (auto const& cluster_desc : cluster_desc_) {
    auto cluster = std::make_shared<storages::postgres::Cluster>(
        cluster_desc, *bg_task_processor_, min_pool_size_, max_pool_size_,
        cmd_ctl);
    clusters_.push_back(cluster);
    tasks.push_back(cluster->DiscoverTopology());
  }

  // Wait for topology discovery
  LOG_DEBUG() << "Wait for initial topology discovery";
  for (auto& t : tasks) {
    // An exception in topology discovery at this stage should prevent starting
    // the component
    t.Get();
  }
  LOG_DEBUG() << "Component ready";
}

Postgres::~Postgres() {
  statistics_holder_.Unregister();
  config_subscription_.Unsubscribe();
}

storages::postgres::ClusterPtr Postgres::GetCluster() const {
  return GetClusterForShard(kDefaultShardNumber);
}

storages::postgres::ClusterPtr Postgres::GetClusterForShard(
    size_t shard) const {
  if (shard >= GetShardCount()) {
    throw storages::postgres::ClusterUnavailable(
        "Shard number " + std::to_string(shard) + " is out of range");
  }

  return clusters_[shard];
}

size_t Postgres::GetShardCount() const { return clusters_.size(); }

formats::json::Value Postgres::ExtendStatistics(
    const utils::statistics::StatisticsRequest& /*request*/) {
  std::vector<storages::postgres::Cluster*> shards_ready;
  shards_ready.reserve(GetShardCount());
  std::transform(clusters_.begin(), clusters_.end(),
                 std::back_inserter(shards_ready),
                 [](const auto& sh) { return sh.get(); });

  formats::json::ValueBuilder result(formats::json::Type::kObject);
  result[db_name_] = PostgresStatisticsToJson(shards_ready);
  return result.ExtractValue();
}

void Postgres::SetDefaultCommandControl(
    storages::postgres::CommandControl cmd_ctl) {
  for (auto cluster : clusters_) {
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
