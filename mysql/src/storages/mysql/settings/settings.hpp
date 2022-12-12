#pragma once

#include <unordered_map>
#include <vector>

#include <userver/components/component_fwd.hpp>
#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::settings {

struct AuthSettings final {
  std::string database;
  std::string user;
  std::string password;
};

AuthSettings Parse(const formats::json::Value& doc,
                   formats::parse::To<AuthSettings>);

struct EndpointInfo final {
  std::string host;
  std::uint32_t port;
};

struct PoolSettings final {
  std::size_t initial_pool_size{1};
  std::size_t max_pool_size{10};

  EndpointInfo endpoint_info;
  AuthSettings auth_settings;

  PoolSettings(const components::ComponentConfig& config,
               const EndpointInfo& endpoint_info, const AuthSettings& auth);
};

struct MysqlSettings final {
  std::vector<EndpointInfo> endpoints;

  AuthSettings auth;

  explicit MysqlSettings(const formats::json::Value& database);
};

class MysqlSettingsMulti final {
 public:
  explicit MysqlSettingsMulti(const formats::json::Value& secdist);

  const MysqlSettings& Get(const std::string& dbname) const;

 private:
  std::unordered_map<std::string, MysqlSettings> databases_;
};

std::string GetSecdistAlias(const components::ComponentConfig& config);

}  // namespace storages::mysql::settings

USERVER_NAMESPACE_END
