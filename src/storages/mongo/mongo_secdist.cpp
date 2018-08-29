#include "mongo_secdist.hpp"

#include <storages/secdist/exceptions.hpp>
#include <storages/secdist/helpers.hpp>

#include <json/value.h>

namespace storages {
namespace mongo {
namespace secdist {

MongoSettings::MongoSettings(const Json::Value& doc) {
  const Json::Value& mongo_settings = doc["mongo_settings"];
  storages::secdist::CheckIsObject(mongo_settings, "mongo_settings");

  for (auto it = mongo_settings.begin(); it != mongo_settings.end(); ++it) {
    const std::string& dbalias = it.key().asString();
    const Json::Value& dbsettings = *it;
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

}  // namespace secdist
}  // namespace mongo
}  // namespace storages
