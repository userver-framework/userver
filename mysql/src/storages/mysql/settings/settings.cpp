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

AuthSettings::~AuthSettings() {
  std::memset(password.GetUnderlying().data(), 0, password.size());
}

AuthSettings Parse(const formats::json::Value& doc,
                   formats::parse::To<AuthSettings>) {
  AuthSettings auth{};
  auth.database = doc["database"].As<std::string>();
  auth.user = doc["user"].As<std::string>();
  auth.password = decltype(auth.password){doc["password"].As<std::string>()};

  return auth;
}

ConnectionSettings Parse(const yaml_config::YamlConfig& doc,
                         formats::parse::To<ConnectionSettings>) {
  ConnectionSettings settings{};
  // TODO : named const
  settings.statements_cache_size =
      doc["statements_cache_size"].As<std::size_t>(20);
  // TODO
  settings.use_secure_connection = false;
  // TODO
  settings.use_compression = false;
  // TODO
  settings.ip_mode = IpMode::kIpV4;

  return settings;
}

PoolSettings PoolSettings::Create(const components::ComponentConfig& config,
                                  const EndpointInfo& endpoint_info,
                                  const AuthSettings& auth_settings) {
  PoolSettings settings{};
  settings.initial_pool_size =
      config["initial_pool_size"].As<std::size_t>(settings.initial_pool_size);
  settings.max_pool_size =
      config["max_pool_size"].As<std::size_t>(settings.max_pool_size);
  settings.endpoint_info = endpoint_info;
  settings.auth_settings = auth_settings;
  settings.connection_settings = config.As<ConnectionSettings>();

  UINVARIANT(
      settings.max_pool_size >= settings.initial_pool_size,
      "max_pool_size should be >= initial_pool_size, recheck your config");

  return settings;
}

MysqlSettings Parse(const formats::json::Value& value,
                    formats::parse::To<MysqlSettings>) {
  auto port = value["port"].As<std::uint32_t>();
  auto hosts = ParseHosts(value);

  MysqlSettings settings{};

  settings.endpoints.reserve(hosts.size());
  for (auto& host : hosts) {
    settings.endpoints.push_back(EndpointInfo{std::move(host), port});
  }

  settings.auth = value.As<AuthSettings>();
  return settings;
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
