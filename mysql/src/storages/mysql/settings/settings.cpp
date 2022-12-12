#include <storages/mysql/settings/settings.hpp>

#include <stdexcept>
#include <unordered_set>
#include <vector>

#include <fmt/format.h>

#include <userver/components/component_config.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/storages/secdist/helpers.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::settings {

namespace {

std::vector<std::string> ParseHosts(const formats::json::Value& database) {
  const auto hosts_json = database["hosts"];
  auto hosts = hosts_json.As<std::vector<std::string>>();

  UINVARIANT(!hosts.empty(), "Empty list of hosts in mysql secdist");
  const auto unique_count =
      std::unordered_set<std::string>{hosts.begin(), hosts.end()}.size();
  UINVARIANT(unique_count == hosts.size(),
             "Hosts are not unique in mysql secdist");

  return hosts;
}

}  // namespace

AuthSettings Parse(const formats::json::Value& doc,
                   formats::parse::To<AuthSettings>) {
  AuthSettings auth{};
  auth.database = doc["database"].As<std::string>();
  auth.user = doc["user"].As<std::string>();
  auth.password = doc["password"].As<std::string>();

  return auth;
}

PoolSettings::PoolSettings(const components::ComponentConfig& config,
                           const EndpointInfo& endpoint_info,
                           const AuthSettings& auth_settings)
    : initial_pool_size{std::max(config["initial_pool_size"].As<std::size_t>(1),
                                 std::size_t{1})},
      // TODO : validate against min_pool_size
      max_pool_size{config["max_pool_size"].As<std::size_t>(10)},
      endpoint_info{endpoint_info},
      auth_settings{auth_settings} {}

MysqlSettings::MysqlSettings(const formats::json::Value& database) {
  auto port = database["port"].As<std::uint32_t>();
  auto hosts = ParseHosts(database);

  endpoints.reserve(hosts.size());
  for (auto& host : hosts) {
    endpoints.push_back(EndpointInfo{std::move(host), port});
  }

  auth = database.As<AuthSettings>();
}

MysqlSettings Parse(const formats::json::Value& value,
                    formats::parse::To<MysqlSettings>) {
  return MysqlSettings{value};
}

MysqlSettingsMulti::MysqlSettingsMulti(const formats::json::Value& secdist) {
  const auto mysql_settings = secdist["mysql_settings"];
  secdist::CheckIsObject(mysql_settings, "mysql_settings");

  databases_ =
      mysql_settings.As<std::unordered_map<std::string, MysqlSettings>>();
}

const MysqlSettings& MysqlSettingsMulti::Get(const std::string& dbname) const {
  const auto it = databases_.find(dbname);
  if (it == databases_.end()) {
    throw std::runtime_error{
        fmt::format("Database '{}' is not found in secdist", dbname)};
  }

  return it->second;
}

std::string GetSecdistAlias(const components::ComponentConfig& config) {
  return config["secdist_alias"].As<std::string>(config.Name());
}

}  // namespace storages::mysql::settings

USERVER_NAMESPACE_END
