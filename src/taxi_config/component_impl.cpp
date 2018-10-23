#include "component_impl.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>

#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/types.hpp>
#include <mongocxx/options/find.hpp>
#include <mongocxx/read_preference.hpp>

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
  auto& mongo_component = context.FindComponent<Mongo>("mongo-taxi");
  mongo_taxi_ = mongo_component.GetPool();
  fallback_path_ = config.ParseString("fallback_path");
}

void TaxiConfigImpl::Update(CacheUpdateTrait::UpdateType type,
                            const std::chrono::system_clock::time_point&,
                            const std::chrono::system_clock::time_point&,
                            tracing::Span&& span) {
  namespace bbb = bsoncxx::builder::basic;
  namespace sm = storages::mongo;
  namespace config_db = mongo::db::taxi::config;

  auto collection = mongo_taxi_->GetCollection(config_db::kCollection);

  if (type == CacheUpdateTrait::UpdateType::kIncremental) {
    auto query = bbb::make_document(
        bbb::kvp(config_db::kUpdated, [this](bbb::sub_document subdoc) {
          subdoc.append(
              bbb::kvp("$gt", bsoncxx::types::b_date(seen_doc_update_time_)));
        }));
    mongocxx::read_preference read_pref;
    read_pref.mode(mongocxx::read_preference::read_mode::k_secondary_preferred);
    mongocxx::options::find options;
    options.read_preference(std::move(read_pref));
    if (!collection
             .FindOne(std::move(query), {config_db::kUpdated},
                      std::move(options))
             .Get()) {
      LOG_DEBUG() << "No changes in incremental config update";
      return;
    }
    TRACE_DEBUG(span) << "Updating dirty config";
  } else {
    TRACE_DEBUG(span) << "Full config update";
  }

  taxi_config::DocsMap mongo_docs;
  try {
    mongo_docs.Parse(ReadFile(fallback_path_));
  } catch (const std::exception& ex) {
    throw std::runtime_error(std::string("Cannot load fallback taxi config: ") +
                             ex.what());
  }

  std::chrono::system_clock::time_point seen_doc_update_time;
  for (const auto& doc : collection.Find(sm::kEmptyObject).Get()) {
    const auto& updated_field = doc[config_db::kUpdated];
    if (updated_field) {
      seen_doc_update_time =
          std::max(seen_doc_update_time, sm::ToTimePoint(updated_field));
    }
    mongo_docs.Set(sm::ToString(doc[config_db::kId]), sm::DocumentValue(doc));
  }

  emplace_docs_cb_(std::move(mongo_docs));
  seen_doc_update_time_ = seen_doc_update_time;
}

}  // namespace components
