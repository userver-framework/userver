#include <storages/mongo/component.hpp>

#include <stdexcept>

#include <components/manager.hpp>
#include <logging/log.hpp>
#include <storages/mongo/error.hpp>
#include <storages/secdist/component.hpp>

#include <engine/task/task_processor.hpp>
#include <engine/task/task_processor_config.hpp>
#include <storages/mongo/mongo_secdist.hpp>
#include <storages/secdist/exceptions.hpp>

namespace components {

namespace {

const std::string kThreadName = "mongo-worker";

auto MakeTaskProcessor(const ComponentConfig& config,
                       const ComponentContext& context,
                       size_t default_threads_num) {
  const auto threads_num = config.ParseUint64("threads", default_threads_num);
  engine::TaskProcessorConfig task_processor_config;
  task_processor_config.thread_name = kThreadName;
  task_processor_config.worker_threads = threads_num;
  return std::make_unique<engine::TaskProcessor>(
      std::move(task_processor_config),
      context.GetManager().GetTaskProcessorPools());
}

std::string GetSecdistConnectionString(const Secdist& secdist,
                                       const std::string& dbalias) {
  try {
    return secdist.Get()
        .Get<storages::mongo::secdist::MongoSettings>()
        .GetConnectionString(dbalias);
  } catch (const storages::secdist::SecdistError& ex) {
    std::string message =
        "Failed to load mongo config for dbalias " + dbalias + ": " + ex.what();
    LOG_ERROR() << message;
    throw storages::mongo::InvalidConfig(std::move(message));
  }
}

}  // namespace

Mongo::Mongo(const ComponentConfig& config, const ComponentContext& context)
    : LoggableComponentBase(config, context) {
  auto dbalias = config.ParseString("dbalias", {});

  std::string connection_string;
  if (!dbalias.empty()) {
    connection_string =
        GetSecdistConnectionString(context.FindComponent<Secdist>(), dbalias);
  } else {
    connection_string = config.ParseString("dbconnection");
  }

  storages::mongo::PoolConfig pool_config(config);

  task_processor_ = MakeTaskProcessor(config, context, kDefaultThreadsNum);

  pool_ = std::make_shared<storages::mongo::Pool>(
      connection_string, *task_processor_, pool_config);
}

Mongo::~Mongo() = default;

storages::mongo::PoolPtr Mongo::GetPool() const { return pool_; }

MultiMongo::MultiMongo(const ComponentConfig& config,
                       const ComponentContext& context)
    : LoggableComponentBase(config, context),
      secdist_(context.FindComponent<Secdist>()),
      pool_config_(config),
      task_processor_(MakeTaskProcessor(config, context, kDefaultThreadsNum)),
      pool_map_ptr_(std::make_shared<PoolMap>()) {}

MultiMongo::~MultiMongo() = default;

storages::mongo::PoolPtr MultiMongo::GetPool(const std::string& dbalias) const {
  auto pool_ptr = FindPool(dbalias);
  if (!pool_ptr)
    throw storages::mongo::PoolNotFound("pool " + dbalias +
                                        " is not in the working set");
  return pool_ptr;
}

void MultiMongo::AddPool(std::string dbalias) {
  auto set = NewPoolSet();
  set.AddExistingPools();
  set.AddPool(std::move(dbalias));
  set.Activate();
}

bool MultiMongo::RemovePool(const std::string& dbalias) {
  if (!FindPool(dbalias)) return false;

  auto set = NewPoolSet();
  set.AddExistingPools();
  if (set.RemovePool(dbalias)) {
    set.Activate();
    return true;
  }
  return false;
}

MultiMongo::PoolSet MultiMongo::NewPoolSet() { return PoolSet(*this); }

storages::mongo::PoolPtr MultiMongo::FindPool(
    const std::string& dbalias) const {
  auto pool_map = pool_map_ptr_.Get();
  assert(pool_map);

  auto it = pool_map->find(dbalias);
  if (it == pool_map->end()) return {};
  return it->second;
}

MultiMongo::PoolSet::PoolSet(MultiMongo& target)
    : target_(&target), pool_map_ptr_(std::make_shared<PoolMap>()) {}

MultiMongo::PoolSet::PoolSet(const PoolSet& other) { *this = other; }

MultiMongo::PoolSet::PoolSet(PoolSet&&) noexcept = default;

MultiMongo::PoolSet& MultiMongo::PoolSet::operator=(const PoolSet& rhs) {
  if (this == &rhs) return *this;

  target_ = rhs.target_;
  pool_map_ptr_ = std::make_shared<PoolMap>(*rhs.pool_map_ptr_);
  return *this;
}

MultiMongo::PoolSet& MultiMongo::PoolSet::operator=(PoolSet&&) noexcept =
    default;

void MultiMongo::PoolSet::AddExistingPools() {
  auto pool_map = target_->pool_map_ptr_.Get();
  assert(pool_map);

  pool_map_ptr_->insert(pool_map->begin(), pool_map->end());
}

void MultiMongo::PoolSet::AddPool(std::string dbalias) {
  auto pool_ptr = target_->FindPool(dbalias);

  if (!pool_ptr) {
    pool_ptr = std::make_shared<storages::mongo::Pool>(
        GetSecdistConnectionString(target_->secdist_, dbalias),
        *target_->task_processor_, target_->pool_config_);
  }

  pool_map_ptr_->emplace(std::move(dbalias), std::move(pool_ptr));
}

bool MultiMongo::PoolSet::RemovePool(const std::string& dbalias) {
  return pool_map_ptr_->erase(dbalias);
}

void MultiMongo::PoolSet::Activate() {
  target_->pool_map_ptr_.Set(pool_map_ptr_);
}

}  // namespace components
