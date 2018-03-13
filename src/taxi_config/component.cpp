#include "component.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>

#include <mongo/client/dbclientinterface.h>

#include <storages/mongo/component.hpp>
#include <storages/mongo/mongo.hpp>
#include <storages/mongo/names.hpp>

#include "config.hpp"
#include "value.hpp"

namespace components {

namespace {

std::string ReadFile(const std::string& path) {
  std::ifstream ifs(path);
  std::ostringstream buffer;
  buffer << ifs.rdbuf();
  return buffer.str();
}

}  // namespace

TaxiConfig::TaxiConfig(const ComponentConfig& config,
                       const ComponentContext& context)
    : CachingComponentBase(config, name) {
  auto mongo_component = context.FindComponent<Mongo>("mongo-taxi");
  if (!mongo_component) {
    throw std::runtime_error("Taxi config requires mongo-taxi component");
  }
  mongo_taxi_ = mongo_component->GetPool();
  fallback_path_ = config.ParseString("fallback_path");

  StartPeriodicUpdates();
}

TaxiConfig::~TaxiConfig() { StopPeriodicUpdates(); }

void TaxiConfig::Update(UpdateType type,
                        const std::chrono::system_clock::time_point&,
                        const std::chrono::system_clock::time_point&) {
  namespace sm = storages::mongo;
  namespace config_db = sm::db::Taxi::Config;

  auto collection = mongo_taxi_->GetCollection(config_db::kCollection);

  if (type == UpdateType::kIncremental) {
    auto query = MONGO_QUERY(config_db::kUpdated
                             << BSON("$gt" << sm::Date(seen_doc_update_time_)))
                     .readPref(mongo::ReadPreference_SecondaryPreferred,
                               sm::kEmptyArray);
    if (collection.FindOne(query, {config_db::kUpdated}).isEmpty()) {
      return;
    }
    LOG_DEBUG() << "Updating dirty config";
  } else {
    LOG_DEBUG() << "Full config update";
  }

  taxi_config::DocsMap mongo_docs;
  mongo_docs.Parse(ReadFile(fallback_path_));

  std::chrono::system_clock::time_point seen_doc_update_time;
  auto cursor = collection.Find(::mongo::Query());
  while (cursor.More()) {
    const auto& doc = cursor.NextSafe();
    seen_doc_update_time = std::max(seen_doc_update_time,
                                    sm::ToTimePoint(doc[config_db::kUpdated]));
    mongo_docs.Set(sm::ToString(doc[config_db::kId]), doc.getOwned());
  }

  Emplace(mongo_docs);
  seen_doc_update_time_ = seen_doc_update_time;
}

}  // namespace components
