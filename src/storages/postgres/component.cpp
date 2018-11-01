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

const std::string kThreadSuffix = "-bg-worker";

}  // namespace

namespace components {

Postgres::Postgres(const ComponentConfig& config,
                   const ComponentContext& context)
    : LoggableComponentBase(config, context) {
  const auto dbalias = config.ParseString("dbalias", {});

  storages::postgres::ClusterDescription cluster_desc;
  if (dbalias.empty()) {
    const auto dsn_string = config.ParseString("dbconnection");
    cluster_desc = storages::postgres::ClusterDescription(dsn_string);
  } else {
    try {
      auto& secdist = context.FindComponent<Secdist>();
      cluster_desc = secdist.Get()
                         .Get<storages::postgres::secdist::PostgresSettings>()
                         .GetClusterDescription(dbalias);
    } catch (const storages::secdist::SecdistError& ex) {
      LOG_ERROR() << "Failed to load Postgres config for dbalias " << dbalias
                  << ": " << ex.what();
      throw;
    }
  }

  const auto min_pool_size =
      config.ParseUint64("min_pool_size", kDefaultMinPoolSize);
  const auto max_pool_size =
      config.ParseUint64("max_pool_size", kDefaultMaxPoolSize);

  const auto threads_num =
      config.ParseUint64("blocking_task_processor_threads");
  engine::TaskProcessorConfig task_processor_config;
  task_processor_config.thread_name = config.Name() + kThreadSuffix;
  task_processor_config.worker_threads = threads_num;
  bg_task_processor_ = std::make_unique<engine::TaskProcessor>(
      std::move(task_processor_config),
      context.GetManager().GetTaskProcessorPools());

  cluster_ = std::make_shared<storages::postgres::Cluster>(
      cluster_desc, *bg_task_processor_, min_pool_size, max_pool_size);
}

Postgres::~Postgres() = default;

storages::postgres::ClusterPtr Postgres::GetCluster() const { return cluster_; }

}  // namespace components
