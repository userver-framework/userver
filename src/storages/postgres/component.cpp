#include <storages/postgres/component.hpp>

#include <components/manager.hpp>
#include <engine/task/task_processor.hpp>
#include <engine/task/task_processor_config.hpp>
#include <logging/log.hpp>
#include <storages/secdist/component.hpp>
#include <storages/secdist/exceptions.hpp>

#include <storages/postgres/cluster.hpp>
#include <storages/postgres/cluster_types.hpp>
#include <storages/postgres/dsn.hpp>
#include <storages/postgres/exceptions.hpp>
#include <storages/postgres/postgres_secdist.hpp>

namespace components {

Postgres::Postgres(const ComponentConfig& config,
                   const ComponentContext& context)
    : LoggableComponentBase(config, context) {
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

  shards_.resize(shard_to_desc_.size());

  min_pool_size_ = config.ParseUint64("min_pool_size", kDefaultMinPoolSize);
  max_pool_size_ = config.ParseUint64("max_pool_size", kDefaultMaxPoolSize);

  const auto task_processor_name =
      config.ParseString("blocking_task_processor");
  bg_task_processor_ = context.GetTaskProcessor(task_processor_name);
  if (!bg_task_processor_) {
    throw storages::postgres::InvalidConfig(
        "Blocking task processor is not set");
  }
}

Postgres::~Postgres() = default;

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
  }
  return cluster;
}

size_t Postgres::GetShardCount() const { return shards_.size(); }

}  // namespace components
