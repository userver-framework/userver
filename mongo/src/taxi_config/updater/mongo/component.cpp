#include <taxi_config/updater/mongo/component.hpp>

#include <stdexcept>

#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/json.hpp>
#include <bsoncxx/types.hpp>
#include <formats/json/serialize.hpp>
#include <fs/blocking/read.hpp>
#include <mongocxx/options/find.hpp>
#include <mongocxx/read_preference.hpp>
#include <storages/mongo/component.hpp>
#include <storages/mongo/mongo.hpp>
#include <taxi_config/mongo/names.hpp>

namespace components {

TaxiConfigMongoUpdater::TaxiConfigMongoUpdater(const ComponentConfig& config,
                                               const ComponentContext& context)
    : CachingComponentBase(config, context, kName),
      taxi_config_(context.FindComponent<TaxiConfig>()) {
  auto& mongo_component = context.FindComponent<Mongo>("mongo-taxi");
  mongo_taxi_ = mongo_component.GetPool();

  auto fallback_config_contents =
      fs::blocking::ReadFileContents(config.ParseString("fallback-path"));
  try {
    fallback_config_.Parse(fallback_config_contents, false);
  } catch (const std::exception& ex) {
    throw std::runtime_error(std::string("Cannot load fallback taxi config: ") +
                             ex.what());
  }

  StartPeriodicUpdates();
}

TaxiConfigMongoUpdater::~TaxiConfigMongoUpdater() { StopPeriodicUpdates(); }

void TaxiConfigMongoUpdater::Update(
    cache::UpdateType type, const std::chrono::system_clock::time_point&,
    const std::chrono::system_clock::time_point&,
    cache::UpdateStatisticsScope& stats) {
  namespace bbb = bsoncxx::builder::basic;
  namespace sm = storages::mongo;
  namespace config_db = mongo::db::taxi::config;

  auto collection = mongo_taxi_->GetCollection(config_db::kCollection);

  if (type == cache::UpdateType::kIncremental) {
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
      stats.FinishNoChanges();
      return;
    }
    LOG_DEBUG() << "Updating dirty config";
  } else {
    LOG_DEBUG() << "Full config update";
  }

  auto mongo_docs = fallback_config_;

  std::chrono::system_clock::time_point seen_doc_update_time;
  for (const auto& doc : collection.Find(sm::kEmptyObject).Get()) {
    stats.IncreaseDocumentsReadCount(1);
    try {
      const auto& updated_field = doc[config_db::kUpdated];
      if (updated_field) {
        seen_doc_update_time =
            std::max(seen_doc_update_time, sm::ToTimePoint(updated_field));
      }
      mongo_docs.Set(sm::ToString(doc[config_db::kId]),
                     formats::json::FromString(bsoncxx::to_json(doc)));
    } catch (const std::exception& e) {
      stats.IncreaseDocumentsParseFailures(1);
    }
  }

  Emplace(std::move(mongo_docs));

  taxi_config_.SetConfig(Get());
  stats.Finish(mongo_docs.Size());
  seen_doc_update_time_ = seen_doc_update_time;
}

}  // namespace components
