#include <taxi_config/updater/mongo/component.hpp>

#include <stdexcept>

#include <formats/bson.hpp>
#include <formats/bson/serialize.hpp>
#include <formats/json/serialize.hpp>
#include <fs/blocking/read.hpp>
#include <storages/mongo/component.hpp>
#include <storages/mongo/options.hpp>
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
  namespace options = storages::mongo::options;
  namespace config_db = mongo::db::taxi::config;

  auto collection = mongo_taxi_->GetCollection(config_db::kCollection);

  if (type == cache::UpdateType::kIncremental) {
    auto query = formats::bson::MakeDoc(
        config_db::kUpdated,
        formats::bson::MakeDoc("$gt", seen_doc_update_time_));
    if (!collection.FindOne(std::move(query),
                            options::Projection{config_db::kUpdated},
                            options::ReadPreference::kSecondaryPreferred)) {
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
  for (const auto& doc : collection.Find({})) {
    stats.IncreaseDocumentsReadCount(1);
    try {
      const auto& updated_field = doc[config_db::kUpdated];
      if (!updated_field.IsMissing()) {
        seen_doc_update_time =
            std::max(seen_doc_update_time,
                     updated_field.As<std::chrono::system_clock::time_point>());
      }
      mongo_docs.Set(doc[config_db::kId].ConvertTo<std::string>(),
                     formats::json::FromString(
                         formats::bson::ToLegacyJsonString(doc).GetView()));
    } catch (const std::exception& e) {
      stats.IncreaseDocumentsParseFailures(1);
    }
  }

  auto docs_count = mongo_docs.Size();
  Emplace(std::move(mongo_docs));

  taxi_config_.SetConfig(Get());
  stats.Finish(docs_count);
  seen_doc_update_time_ = seen_doc_update_time;
}

}  // namespace components
