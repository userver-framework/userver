#include "mongo_secdist.hpp"

#include <storages/secdist/exceptions.hpp>
#include <storages/secdist/helpers.hpp>

namespace storages::mongo::secdist {

MongoSettings::MongoSettings(const formats::json::Value& doc) {
  const formats::json::Value& mongo_settings = doc["mongo_settings"];
  // Mongo library can be used just for BSON without mongo configuration
  if (mongo_settings.IsMissing()) return;

  storages::secdist::CheckIsObject(mongo_settings, "mongo_settings");

  for (auto it = mongo_settings.begin(); it != mongo_settings.end(); ++it) {
    const std::string& dbalias = it.GetName();
    const formats::json::Value& dbsettings = *it;
    storages::secdist::CheckIsObject(dbsettings, "dbsettings");
    settings_[dbalias] = storages::secdist::GetString(dbsettings, "uri");
  }
}

const std::string& MongoSettings::GetConnectionString(
    const std::string& dbalias) const {
  auto it = settings_.find(dbalias);

  if (it == settings_.end())
    throw storages::secdist::UnknownMongoDbAlias(
        "dbalias " + dbalias + " not found in secdist config");

  return it->second;
}

}  // namespace storages::mongo::secdist
