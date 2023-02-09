#include "settings.hpp"

#include <unordered_set>

#include <userver/components/component_config.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/storages/secdist/helpers.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/trivial_map.hpp>

#include <fmt/format.h>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::impl {

namespace {
ConnectionSettings::ConnectionMode GetConnectionMode(
    bool use_secure_connection) {
  return use_secure_connection ? ConnectionSettings::ConnectionMode::kSecure
                               : ConnectionSettings::ConnectionMode::kNonSecure;
}

std::vector<std::string> ParseHosts(const formats::json::Value& doc) {
  const auto hosts_json = doc["hosts"];
  auto hosts = hosts_json.As<std::vector<std::string>>();

  UINVARIANT(!hosts.empty(), "Empty list of hosts in clickhouse secdist");
  const auto unique_count =
      std::unordered_set<std::string>{hosts.begin(), hosts.end()}.size();

  UINVARIANT(unique_count == hosts.size(),
             "Hosts are not unique in clickhouse secdist");

  return hosts;
}

using CompressionMethod = ConnectionSettings::CompressionMethod;

}  // namespace

static CompressionMethod Parse(const yaml_config::YamlConfig& value,
                               formats::parse::To<CompressionMethod>) {
  static constexpr utils::TrivialBiMap kMap([](auto selector) {
    return selector()
        .Case(CompressionMethod::kNone, "none")
        .Case(CompressionMethod::kLZ4, "lz4");
  });

  return utils::ParseFromValueString(value, kMap);
}

AuthSettings::AuthSettings() = default;

AuthSettings::AuthSettings(const formats::json::Value& doc)
    : user{doc["user"].As<std::string>()},
      password{doc["password"].As<std::string>()},
      database{doc["dbname"].As<std::string>()} {}

ConnectionSettings::ConnectionSettings(
    const components::ComponentConfig& config)
    : connection_mode{GetConnectionMode(
          config["use_secure_connection"].As<bool>(true))},
      compression_method{config["compression"].As<CompressionMethod>(
          CompressionMethod::kNone)} {}

PoolSettings::PoolSettings(const components::ComponentConfig& config,
                           const EndpointSettings& endpoint,
                           const AuthSettings& auth)
    : initial_pool_size{std::max(config["initial_pool_size"].As<std::size_t>(5),
                                 std::size_t{1})},
      max_pool_size{config["max_pool_size"].As<size_t>(10)},
      queue_timeout{config["queue_timeout"].As<std::chrono::milliseconds>(
          std::chrono::milliseconds{200})},
      endpoint_settings{endpoint},
      auth_settings{auth},
      connection_settings{config} {}

ClickhouseSettings::ClickhouseSettings() = default;

ClickhouseSettings::ClickhouseSettings(const formats::json::Value& doc) {
  auto port = doc["port"].As<uint32_t>();
  auto hosts = ParseHosts(doc);

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

std::string GetSecdistAlias(const components::ComponentConfig& config) {
  return config.HasMember("secdist_alias")
             ? config["secdist_alias"].As<std::string>()
             : config.Name();
}

}  // namespace storages::clickhouse::impl

USERVER_NAMESPACE_END
