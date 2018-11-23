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
#include <storages/postgres/postgres_secdist.hpp>
#include <storages/postgres/statistics.hpp>

namespace {

constexpr auto kStatisticsName = "postgresql";

formats::json::ValueBuilder InstanceStatisticsToJson(
    const storages::postgres::InstanceStatsDescriptor& desc) {
  if (desc.dsn.empty()) {
    return {};
  }

  const auto& stats = desc.stats;
  formats::json::ValueBuilder instance(formats::json::Type::kObject);
  instance["dsn"] = desc.dsn;

  auto conn = instance["connections"];
  conn["opened"] = stats.conn_open_total;
  conn["closed"] = stats.conn_drop_total;
  conn["active"] = stats.conn_active_total;

  auto trx = instance["transactions"];
  trx["total"] = stats.trx_total;
  trx["committed"] = stats.trx_commit_total;
  trx["rolled-back"] = stats.trx_rollback_total;

  auto timing = trx["timings"];
  timing["delay"]["1min"] =
      utils::statistics::PercentileToJson(stats.trx_delay_percentile);
  timing["wall"]["1min"] =
      utils::statistics::PercentileToJson(stats.trx_exec_percentile);
  timing["user"]["1min"] =
      utils::statistics::PercentileToJson(stats.trx_user_percentile);

  auto query = instance["queries"];
  query["parsed"] = stats.trx_parse_total;
  query["executed"] = stats.trx_execute_total;
  query["replies"] = stats.trx_reply_total;
  query["binary-replies"] = stats.trx_bin_reply_total;

  auto errors = instance["errors"];
  errors["query-exec"] = stats.trx_error_execute_total;
  errors["connection"] = stats.conn_error_total;
  errors["pool"] = stats.pool_error_exhaust_total;

  return instance;
}

formats::json::ValueBuilder ClusterStatisticsToJson(
    const storages::postgres::ClusterStatistics& stats) {
  formats::json::ValueBuilder cluster(formats::json::Type::kObject);
  cluster["master"] = InstanceStatisticsToJson(stats.master);
  cluster["sync_slave"] = InstanceStatisticsToJson(stats.sync_slave);
  auto slaves = cluster["slaves"];
  slaves = {formats::json::Type::kObject};
  for (auto i = 0u; i < stats.slaves.size(); ++i) {
    const auto slave_name = "slave_" + std::to_string(i);
    slaves[slave_name] = InstanceStatisticsToJson(stats.slaves[i]);
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
  const auto dbalias = config.ParseString("dbalias", {});

  storages::postgres::ClusterDescription cluster_desc;
  if (dbalias.empty()) {
    const auto dsn_string = config.ParseString("dbconnection");
    shard_to_desc_.push_back(
        storages::postgres::ClusterDescription(dsn_string));
  } else {
    try {
      auto& secdist = context.FindComponent<Secdist>();
      shard_to_desc_ = secdist.Get()
                           .Get<storages::postgres::secdist::PostgresSettings>()
                           .GetShardedClusterDescription(dbalias);
    } catch (const storages::secdist::SecdistError& ex) {
      LOG_ERROR() << "Failed to load Postgres config for dbalias " << dbalias
                  << ": " << ex.what();
      throw;
    }
  }

  const auto shard_count = shard_to_desc_.size();
  shards_.resize(shard_count);
  shards_ready_.resize(shard_count, nullptr);

  min_pool_size_ = config.ParseUint64("min_pool_size", kDefaultMinPoolSize);
  max_pool_size_ = config.ParseUint64("max_pool_size", kDefaultMaxPoolSize);

  const auto task_processor_name =
      config.ParseString("blocking_task_processor");
  bg_task_processor_ = context.GetTaskProcessor(task_processor_name);
  if (!bg_task_processor_) {
    throw storages::postgres::InvalidConfig(
        "Blocking task processor is not set");
  }

  statistics_holder_ = statistics_storage_.GetStorage().RegisterExtender(
      kStatisticsName,
      std::bind(&Postgres::ExtendStatistics, this, std::placeholders::_1));
}

Postgres::~Postgres() { statistics_holder_.Unregister(); }

storages::postgres::ClusterPtr Postgres::GetCluster() const {
  return GetClusterForShard(kDefaultShardNumber);
}

storages::postgres::ClusterPtr Postgres::GetClusterForShard(
    size_t shard) const {
  if (shard >= GetShardCount()) {
    throw storages::postgres::ClusterUnavailable(
        "Shard number " + std::to_string(shard) + " is out of range");
  }

  std::lock_guard<engine::Mutex> lock(shards_mutex_);

  auto& cluster = shards_[shard];
  if (!cluster) {
    const auto& desc = shard_to_desc_[shard];
    cluster = std::make_shared<storages::postgres::Cluster>(
        desc, *bg_task_processor_, min_pool_size_, max_pool_size_);

    std::lock_guard<engine::Mutex> lk(shards_ready_mutex_);
    shards_ready_[shard] = cluster.get();
  }
  return cluster;
}

size_t Postgres::GetShardCount() const { return shards_.size(); }

formats::json::Value Postgres::ExtendStatistics(
    const utils::statistics::StatisticsRequest& /*request*/) {
  std::vector<storages::postgres::Cluster*> shards_ready;
  shards_ready.reserve(GetShardCount());
  {
    std::lock_guard<engine::Mutex> lk(shards_ready_mutex_);
    shards_ready = shards_ready_;
  }
  return PostgresStatisticsToJson(shards_ready).ExtractValue();
}

}  // namespace components
