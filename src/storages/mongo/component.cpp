#include <storages/mongo/component.hpp>

#include <stdexcept>

#include <components/manager.hpp>
#include <logging/log.hpp>
#include <storages/secdist/component.hpp>

#include <engine/task/task_processor.hpp>
#include <engine/task/task_processor_config.hpp>
#include <storages/secdist/exceptions.hpp>
#include "mongo_secdist.hpp"

namespace components {

namespace {

const std::string kThreadName = "mongo-worker";

}  // namespace

Mongo::Mongo(const ComponentConfig& config, const ComponentContext& context) {
  auto dbalias = config.ParseString("dbalias", {});

  std::string connection_string;
  if (!dbalias.empty()) {
    try {
      auto* secdist = context.FindComponent<Secdist>();
      if (!secdist)
        throw std::runtime_error("Mongo requires secdist component");
      connection_string = secdist->Get()
                              .Get<storages::mongo::secdist::MongoSettings>()
                              .GetConnectionString(dbalias);
    } catch (const storages::secdist::SecdistError& ex) {
      LOG_ERROR() << "Failed to load mongo config for dbalias " << dbalias
                  << ": " << ex.what();
      throw;
    }
  } else {
    connection_string = config.ParseString("dbconnection");
  }

  const auto conn_timeout_ms =
      config.ParseInt("conn_timeout_ms", kDefaultConnTimeoutMs);
  const auto so_timeout_ms =
      config.ParseInt("so_timeout_ms", kDefaultSoTimeoutMs);
  const auto min_pool_size =
      config.ParseUint64("min_pool_size", kDefaultMinPoolSize);
  const auto max_pool_size =
      config.ParseUint64("max_pool_size", kDefaultMaxPoolSize);

  const auto threads_num = config.ParseUint64("threads", kDefaultThreadsNum);
  engine::TaskProcessorConfig task_processor_config;
  task_processor_config.thread_name = kThreadName;
  task_processor_config.worker_threads = threads_num;
  task_processor_ = std::make_unique<engine::TaskProcessor>(
      std::move(task_processor_config), context.GetManager().GetCoroPool(),
      context.GetManager().GetEventThreadPool());

  pool_ = std::make_shared<storages::mongo::Pool>(
      connection_string, *task_processor_, conn_timeout_ms, so_timeout_ms,
      min_pool_size, max_pool_size);
}

Mongo::~Mongo() = default;

storages::mongo::PoolPtr Mongo::GetPool() const { return pool_; }

}  // namespace components
