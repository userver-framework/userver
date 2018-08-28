#include "component_impl.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>

#include <mongo/client/dbclientinterface.h>

#include <storages/mongo/component.hpp>
#include <storages/mongo/mongo.hpp>

#include <taxi_config/mongo/names.hpp>

namespace components {

namespace {

std::string ReadFile(const std::string& path) {
  std::ifstream ifs(path);
  if (!ifs) {
    throw std::runtime_error("Error opening '" + path + '\'');
  }

  std::ostringstream buffer;
  buffer << ifs.rdbuf();
  return buffer.str();
}

}  // namespace

TaxiConfigImpl::TaxiConfigImpl(const ComponentConfig& config,
                               const ComponentContext& context,
                               EmplaceDocsCb emplace_docs_cb)
    : emplace_docs_cb_(std::move(emplace_docs_cb)) {
  auto mongo_component = context.FindComponent<Mongo>("mongo-taxi");
  if (!mongo_component) {
    throw std::runtime_error("Taxi config requires mongo-taxi component");
  }
  mongo_taxi_ = mongo_component->GetPool();
  fallback_path_ = config.ParseString("fallback_path");
}

void TaxiConfigImpl::Update(UpdatingComponentBase::UpdateType type,
                            const std::chrono::system_clock::time_point&,
                            const std::chrono::system_clock::time_point&) {
  namespace sm = storages::mongo;
  namespace config_db = mongo::db::taxi::config;

  auto collection = mongo_taxi_->GetCollection(config_db::kCollection);

  if (type == UpdatingComponentBase::UpdateType::kIncremental) {
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
  try {
    mongo_docs.Parse(ReadFile(fallback_path_));
  } catch (const std::exception& ex) {
    throw std::runtime_error(std::string("Cannot load fallback taxi config: ") +
                             ex.what());
  }

  std::chrono::system_clock::time_point seen_doc_update_time;
  auto cursor = collection.Find(::mongo::Query());
  while (cursor.More()) {
    const auto& doc = cursor.NextSafe();
    const auto& updated_field = doc[config_db::kUpdated];
    if (updated_field.ok()) {
      seen_doc_update_time =
          std::max(seen_doc_update_time, sm::ToTimePoint(updated_field));
    }
    mongo_docs.Set(sm::ToString(doc[config_db::kId]), doc.getOwned());
  }

  emplace_docs_cb_(std::move(mongo_docs));
  seen_doc_update_time_ = seen_doc_update_time;
}

}  // namespace components
