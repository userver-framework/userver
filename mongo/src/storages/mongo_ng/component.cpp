#include <storages/mongo_ng/component.hpp>

#include <stdexcept>

#include <components/manager.hpp>
#include <logging/log.hpp>
#include <storages/mongo_ng/exception.hpp>
#include <storages/secdist/component.hpp>

#include <storages/mongo_ng/mongo_secdist.hpp>
#include <storages/secdist/exceptions.hpp>

namespace components {

namespace {

std::string GetSecdistConnectionString(const Secdist& secdist,
                                       const std::string& dbalias) {
  try {
    return secdist.Get()
        .Get<storages::mongo_ng::secdist::MongoSettings>()
        .GetConnectionString(dbalias);
  } catch (const storages::secdist::SecdistError& ex) {
    throw storages::mongo_ng::InvalidConfigException(
        "Failed to load mongo config for dbalias ")
        << dbalias << ": " << ex.what();
  }
}

}  // namespace

MongoNg::MongoNg(const ComponentConfig& config, const ComponentContext& context)
    : LoggableComponentBase(config, context) {
  auto dbalias = config.ParseString("dbalias", {});

  std::string connection_string;
  if (!dbalias.empty()) {
    connection_string =
        GetSecdistConnectionString(context.FindComponent<Secdist>(), dbalias);
  } else {
    connection_string = config.ParseString("dbconnection");
  }

  storages::mongo_ng::PoolConfig pool_config(config);

  pool_ = std::make_shared<storages::mongo_ng::Pool>(
      config.Name(), connection_string, pool_config);
}

storages::mongo_ng::PoolPtr MongoNg::GetPool() const { return pool_; }

MultiMongoNg::MultiMongoNg(const ComponentConfig& config,
                           const ComponentContext& context)
    : LoggableComponentBase(config, context),
      name_(config.Name()),
      secdist_(context.FindComponent<Secdist>()),
      pool_config_(config),
      pool_map_ptr_(std::make_shared<PoolMap>()) {}

storages::mongo_ng::PoolPtr MultiMongoNg::GetPool(
    const std::string& dbalias) const {
  auto pool_ptr = FindPool(dbalias);
  if (!pool_ptr) {
    throw storages::mongo_ng::PoolNotFoundException("pool ")
        << dbalias << " is not in the working set";
  }
  return pool_ptr;
}

void MultiMongoNg::AddPool(std::string dbalias) {
  auto set = NewPoolSet();
  set.AddExistingPools();
  set.AddPool(std::move(dbalias));
  set.Activate();
}

bool MultiMongoNg::RemovePool(const std::string& dbalias) {
  if (!FindPool(dbalias)) return false;

  auto set = NewPoolSet();
  set.AddExistingPools();
  if (set.RemovePool(dbalias)) {
    set.Activate();
    return true;
  }
  return false;
}

MultiMongoNg::PoolSet MultiMongoNg::NewPoolSet() { return PoolSet(*this); }

storages::mongo_ng::PoolPtr MultiMongoNg::FindPool(
    const std::string& dbalias) const {
  auto pool_map = pool_map_ptr_.Get();
  UASSERT(pool_map);

  auto it = pool_map->find(dbalias);
  if (it == pool_map->end()) return {};
  return it->second;
}

MultiMongoNg::PoolSet::PoolSet(MultiMongoNg& target)
    : target_(&target), pool_map_ptr_(std::make_shared<PoolMap>()) {}

MultiMongoNg::PoolSet::PoolSet(const PoolSet& other) { *this = other; }

MultiMongoNg::PoolSet::PoolSet(PoolSet&&) noexcept = default;

MultiMongoNg::PoolSet& MultiMongoNg::PoolSet::operator=(const PoolSet& rhs) {
  if (this == &rhs) return *this;

  target_ = rhs.target_;
  pool_map_ptr_ = std::make_shared<PoolMap>(*rhs.pool_map_ptr_);
  return *this;
}

MultiMongoNg::PoolSet& MultiMongoNg::PoolSet::operator=(PoolSet&&) noexcept =
    default;

void MultiMongoNg::PoolSet::AddExistingPools() {
  auto pool_map = target_->pool_map_ptr_.Get();
  UASSERT(pool_map);

  pool_map_ptr_->insert(pool_map->begin(), pool_map->end());
}

void MultiMongoNg::PoolSet::AddPool(std::string dbalias) {
  auto pool_ptr = target_->FindPool(dbalias);

  if (!pool_ptr) {
    pool_ptr = std::make_shared<storages::mongo_ng::Pool>(
        target_->name_ + ':' + dbalias,
        GetSecdistConnectionString(target_->secdist_, dbalias),
        target_->pool_config_);
  }

  pool_map_ptr_->emplace(std::move(dbalias), std::move(pool_ptr));
}

bool MultiMongoNg::PoolSet::RemovePool(const std::string& dbalias) {
  return pool_map_ptr_->erase(dbalias);
}

void MultiMongoNg::PoolSet::Activate() {
  target_->pool_map_ptr_.Set(pool_map_ptr_);
}

}  // namespace components
