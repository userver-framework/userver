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
#include <storages/postgres/postgres_secdist.hpp>

namespace {

// TODO need different thread names for different components
const std::string kThreadName = "pg-worker";

}  // namespace

namespace components {

Postgres::Postgres(const ComponentConfig& config,
                   const ComponentContext& context)
    : LoggableComponentBase(config, context) {
  const auto dbalias = config.ParseString("dbalias", {});

  storages::postgres::ClusterDescription cluster_desc;
  if (dbalias.empty()) {
    const auto dsn_string = config.ParseString("dbconnection");
    shard_to_desc_[kDefaultShardNumber] =
        storages::postgres::ClusterDescription(dsn_string);
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

  min_pool_size_ = config.ParseUint64("min_pool_size", kDefaultMinPoolSize);
  max_pool_size_ = config.ParseUint64("max_pool_size", kDefaultMaxPoolSize);

  const auto threads_num =
      config.ParseUint64("blocking_task_processor_threads");
  engine::TaskProcessorConfig task_processor_config;
  task_processor_config.thread_name = kThreadName;
  task_processor_config.worker_threads = threads_num;
  bg_task_processor_ = std::make_unique<engine::TaskProcessor>(
      std::move(task_processor_config),
      context.GetManager().GetTaskProcessorPools());
}

Postgres::~Postgres() = default;

storages::postgres::ClusterPtr Postgres::GetCluster() const {
  return GetClusterForShard(kDefaultShardNumber);
}

storages::postgres::ClusterPtr Postgres::GetClusterForShard(
    size_t shard) const {
  std::lock_guard<engine::Mutex> lock(shard_cluster_mutex_);

  auto& cluster = shard_to_cluster_[shard];
  if (!cluster) {
    auto it = shard_to_desc_.find(shard);
    if (it == shard_to_desc_.end()) {
      // TODO find better exception type
      throw storages::secdist::UnknownPostgresDbAlias(
          "Shard number " + std::to_string(shard) + " not found");
    }
    cluster = std::make_shared<storages::postgres::Cluster>(
        it->second, *bg_task_processor_, min_pool_size_, max_pool_size_);
  }
  return cluster;
}

}  // namespace components
