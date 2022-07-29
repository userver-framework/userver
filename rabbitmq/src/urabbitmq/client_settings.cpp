#include <userver/urabbitmq/client_settings.hpp>

#include <userver/formats/json/value.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/storages/secdist/helpers.hpp>
#include <userver/utils/assert.hpp>

#include <fmt/format.h>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

namespace {

std::vector<std::string> ParseHosts(const formats::json::Value& secdist_doc) {
  const auto hosts_json = secdist_doc["hosts"];
  auto hosts = hosts_json.As<std::vector<std::string>>();

  UINVARIANT(!hosts.empty(), "Empty list of hosts in clickhouse secdist");
  const auto unique_count =
      std::unordered_set<std::string>{hosts.begin(), hosts.end()}.size();

  UINVARIANT(unique_count == hosts.size(),
             "Hosts are not unique in clickhouse secdist");

  return hosts;
}

EvPoolType ParseEvPoolType(const components::ComponentConfig& config) {
  auto pool_type = config["ev_pool_type"].As<std::string>();

  if (pool_type == "owned") {
    return EvPoolType::kOwned;
  } else if (pool_type == "shared") {
    return EvPoolType::kShared;
  } else {
    throw std::runtime_error{
        fmt::format("Unknown ev_pool_type '{}'", pool_type)};
  }
}

}  // namespace

AuthSettings::AuthSettings() = default;

AuthSettings::AuthSettings(const formats::json::Value& secdist_doc)
    : login{secdist_doc["login"].As<std::string>()},
      password{secdist_doc["password"].As<std::string>()},
      vhost{secdist_doc["vhost"].As<std::string>()} {}

RabbitEndpoints::RabbitEndpoints() = default;

RabbitEndpoints::RabbitEndpoints(const formats::json::Value& secdist_doc)
    : auth{secdist_doc} {
  auto port = secdist_doc["port"].As<uint16_t>();
  auto hosts = ParseHosts(secdist_doc);

  endpoints.reserve(hosts.size());
  for (auto& host : hosts) {
    endpoints.push_back({std::move(host), port});
  }
}

RabbitEndpoints Parse(const formats::json::Value& doc,
                      formats::parse::To<RabbitEndpoints>) {
  return RabbitEndpoints{doc};
}

ClientSettings::ClientSettings() = default;

ClientSettings::ClientSettings(const components::ComponentConfig& config,
                               const RabbitEndpoints& rabbit_endpoints)
    : ev_pool_type{ParseEvPoolType(config)},
      thread_count{config["thread_count"].As<size_t>(thread_count)},
      connections_per_thread{
          config["connections_per_thread"].As<size_t>(connections_per_thread)},
      channels_per_connection{config["channels_per_connection"].As<size_t>(
          channels_per_connection)},
      secure{config["use_secure_connection"].As<bool>(secure)},
      endpoints{rabbit_endpoints} {}

RabbitEndpointsMulti::RabbitEndpointsMulti(const formats::json::Value& doc) {
  const auto rabbitmq_settings = doc["rabbitmq_settings"];
  storages::secdist::CheckIsObject(rabbitmq_settings, "rabbitmq_settings");

  endpoints_ =
      rabbitmq_settings.As<std::unordered_map<std::string, RabbitEndpoints>>();
}

const RabbitEndpoints& RabbitEndpointsMulti::Get(
    const std::string& name) const {
  const auto it = endpoints_.find(name);
  if (it == endpoints_.end()) {
    throw std::runtime_error{
        fmt::format("RMQ broken '{}' is not found in secdist", name)};
  }

  return it->second;
}

}  // namespace urabbitmq

USERVER_NAMESPACE_END
