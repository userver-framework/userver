#include "secdist.hpp"

#include <userver/formats/json.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/logging/log.hpp>

#include <userver/ydb/exceptions.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb::impl::secdist {
namespace {

DatabaseSettings GetDatabaseSettings(const formats::json::Value& doc) {
  DatabaseSettings settings;
  settings.endpoint = doc["endpoint"].As<std::optional<std::string>>();
  settings.database = doc["database"].As<std::optional<std::string>>();
  settings.sync_start = doc["sync_start"].As<std::optional<bool>>();
  if (doc.HasMember("token")) {
    settings.oauth_token = doc["token"].As<std::string>();
  } else if (doc.HasMember("iam_jwt_params")) {
    settings.iam_jwt_params = doc["iam_jwt_params"];
  }
  return settings;
}

}  // namespace

YdbSettings::YdbSettings(const formats::json::Value& secdist_doc) {
  const auto& ydb_settings = secdist_doc["ydb_settings"];
  if (ydb_settings.IsMissing()) {
    LOG_DEBUG() << "'ydb_settings' secdist section is empty";
    return;
  }
  for (auto it = ydb_settings.begin(); it != ydb_settings.end(); ++it) {
    settings[it.GetName()] = GetDatabaseSettings(*it);
  }
}

}  // namespace ydb::impl::secdist

USERVER_NAMESPACE_END
