#include <storages/mongo/component.hpp>

#include <stdexcept>

#include <boost/algorithm/string/predicate.hpp>

#include <components/manager.hpp>
#include <components/statistics_storage.hpp>
#include <formats/json/value_builder.hpp>
#include <logging/log.hpp>
#include <storages/mongo/exception.hpp>
#include <storages/secdist/component.hpp>

#include <storages/mongo/mongo_secdist.hpp>
#include <storages/secdist/exceptions.hpp>

namespace components {

namespace {

const std::string kStandardMongoPrefix = "mongo-";

std::string GetSecdistConnectionString(const Secdist& secdist,
                                       const std::string& dbalias) {
  try {
    return secdist.Get()
        .Get<storages::mongo::secdist::MongoSettings>()
        .GetConnectionString(dbalias);
  } catch (const storages::secdist::SecdistError& ex) {
    throw storages::mongo::InvalidConfigException(
        "Failed to load mongo config for dbalias ")
        << dbalias << ": " << ex.what();
  }
}

}  // namespace

Mongo::Mongo(const ComponentConfig& config, const ComponentContext& context)
    : LoggableComponentBase(config, context) {
  auto dbalias = config["dbalias"].As<std::string>("");

  std::string connection_string;
  if (!dbalias.empty()) {
    connection_string =
        GetSecdistConnectionString(context.FindComponent<Secdist>(), dbalias);
  } else {
    connection_string = config["dbconnection"].As<std::string>();
  }

  storages::mongo::PoolConfig pool_config(config);

  pool_ = std::make_shared<storages::mongo::Pool>(
      config.Name(), connection_string, pool_config);

  auto& statistics_storage =
      context.FindComponent<components::StatisticsStorage>();

  auto section_name = config.Name();
  if (boost::algorithm::starts_with(section_name, kStandardMongoPrefix) &&
      section_name.size() != kStandardMongoPrefix.size()) {
    section_name = section_name.substr(kStandardMongoPrefix.size());
  }
  statistics_holder_ = statistics_storage.GetStorage().RegisterExtender(
      "mongo." + section_name,
      [this](const auto&) { return pool_->GetStatistics(); });
}

Mongo::~Mongo() { statistics_holder_.Unregister(); }

storages::mongo::PoolPtr Mongo::GetPool() const { return pool_; }

MultiMongo::MultiMongo(const ComponentConfig& config,
                       const ComponentContext& context)
    : LoggableComponentBase(config, context),
      name_(config.Name()),
      secdist_(context.FindComponent<Secdist>()),
      pool_config_(config),
      pool_map_ptr_(std::make_shared<PoolMap>()) {
  auto& statistics_storage =
      context.FindComponent<components::StatisticsStorage>();
  statistics_holder_ = statistics_storage.GetStorage().RegisterExtender(
      name_, [this](const auto&) { return GetStatistics(); });
}

MultiMongo::~MultiMongo() { statistics_holder_.Unregister(); }

storages::mongo::PoolPtr MultiMongo::GetPool(const std::string& dbalias) const {
  auto pool_ptr = FindPool(dbalias);
  if (!pool_ptr) {
    throw storages::mongo::PoolNotFoundException("pool ")
        << dbalias << " is not in the working set";
  }
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
  UASSERT(pool_map);

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
  UASSERT(pool_map);

  pool_map_ptr_->insert(pool_map->begin(), pool_map->end());
}

void MultiMongo::PoolSet::AddPool(std::string dbalias) {
  auto pool_ptr = target_->FindPool(dbalias);

  if (!pool_ptr) {
    pool_ptr = std::make_shared<storages::mongo::Pool>(
        target_->name_ + ':' + dbalias,
        GetSecdistConnectionString(target_->secdist_, dbalias),
        target_->pool_config_);
  }

  pool_map_ptr_->emplace(std::move(dbalias), std::move(pool_ptr));
}

bool MultiMongo::PoolSet::RemovePool(const std::string& dbalias) {
  return pool_map_ptr_->erase(dbalias);
}

void MultiMongo::PoolSet::Activate() {
  target_->pool_map_ptr_.Set(pool_map_ptr_);
}

formats::json::Value MultiMongo::GetStatistics() const {
  formats::json::ValueBuilder builder(formats::json::Type::kObject);

  auto pool_map = pool_map_ptr_.Get();
  for (const auto& [dbalias, pool] : *pool_map) {
    builder[dbalias] = pool->GetStatistics();
  }
  return builder.ExtractValue();
}

}  // namespace components
