#include <userver/storages/clickhouse/settings.hpp>

#include <userver/storages/secdist/helpers.hpp>

#include <userver/components/component_config.hpp>
#include <userver/formats/parse/common_containers.hpp>

#include <fmt/format.h>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse {

AuthSettings::AuthSettings() = default;

AuthSettings::AuthSettings(const formats::json::Value& doc)
    : user{doc["user"].As<std::string>()},
      password{doc["password"].As<std::string>()} {}

void AuthSettings::SetDatabase(const std::string& dbname) { database = dbname; }

PoolSettings::PoolSettings(const components::ComponentConfig& config,
                           const EndpointSettings& endpoint,
                           const AuthSettings& auth)
    : initial_pool_size{config["initial_pool_size"].As<size_t>(5)},
      max_pool_size{config["max_pool_size"].As<size_t>(10)},
      queue_timeout{config["queue_timeout"].As<std::chrono::milliseconds>(
          std::chrono::milliseconds{200})},
      use_secure_connection{config["use_secure_connection"].As<bool>(true)},
      endpoint_settings{endpoint},
      auth_settings{auth} {
  auth_settings.SetDatabase(GetDbName(config));
}

ClickhouseSettings::ClickhouseSettings() = default;

ClickhouseSettings::ClickhouseSettings(const formats::json::Value& doc) {
  auto port = doc["port"].As<uint32_t>();

  const auto hosts_json = doc["hosts"];
  secdist::CheckIsArray(hosts_json, "hosts");
  auto hosts = hosts_json.As<std::vector<std::string>>();
  if (hosts.empty()) {
    throw std::runtime_error{"Empty list of hosts in clickhouse secdist"};
  }

  endpoints.reserve(hosts.size());
  for (auto& host : hosts) {
    endpoints.emplace_back(EndpointSettings{std::move(host), port});
  }

  auth_settings = AuthSettings{doc};
}

ClickhouseSettings Parse(const formats::json::Value& doc,
                         formats::parse::To<ClickhouseSettings>) {
  return ClickhouseSettings{doc};
}

ClickhouseSettingsMulti::ClickhouseSettingsMulti(
    const formats::json::Value& doc) {
  const auto clickhouse_settings = doc["clickhouse_settings"];
  secdist::CheckIsObject(clickhouse_settings, "clickhouse_settings");

  databases_ = clickhouse_settings
                   .As<std::unordered_map<std::string, ClickhouseSettings>>();
}

const ClickhouseSettings& ClickhouseSettingsMulti::Get(
    const std::string& dbname) const {
  const auto it = databases_.find(dbname);
  if (it == databases_.end()) {
    throw std::runtime_error{
        fmt::format("database '{}' is not found in secdist", dbname)};
  }

  return it->second;
}

std::string GetDbName(const components::ComponentConfig& config) {
  return config.HasMember("secdist_alias")
             ? config["secdist_alias"].As<std::string>()
             : config.Name();
}

}  // namespace storages::clickhouse

USERVER_NAMESPACE_END
