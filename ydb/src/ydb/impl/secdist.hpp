#pragma once

#include <string>
#include <unordered_map>

#include <userver/formats/json/value.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb::impl::secdist {

struct DatabaseSettings final {
  std::optional<std::string> endpoint;
  std::optional<std::string> database;
  std::optional<std::string> oauth_token;
  std::optional<formats::json::Value> iam_jwt_params;
  std::optional<bool> sync_start;
};

struct YdbSettings final {
  std::unordered_map<std::string, DatabaseSettings> settings;

  YdbSettings(const formats::json::Value& secdist_doc);
};

}  // namespace ydb::impl::secdist

USERVER_NAMESPACE_END
