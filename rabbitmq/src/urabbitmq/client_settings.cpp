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

PoolSettings ParsePoolSettings(const components::ComponentConfig& config) {
  PoolSettings result{};
  result.min_pool_size =
      config["min_pool_size"].As<size_t>(result.min_pool_size);
  result.max_pool_size =
      config["max_pool_size"].As<size_t>(result.max_pool_size);
  result.max_in_flight_requests = config["max_in_flight_requests"].As<size_t>(
      result.max_in_flight_requests);

  return result;
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

PoolSettings::PoolSettings() = default;

RabbitEndpoints Parse(const formats::json::Value& doc,
                      formats::parse::To<RabbitEndpoints>) {
  return RabbitEndpoints{doc};
}

ClientSettings::ClientSettings() = default;

ClientSettings::ClientSettings(const components::ComponentConfig& config,
                               const RabbitEndpoints& rabbit_endpoints)
    : pool_settings{ParsePoolSettings(config)},
      endpoints{rabbit_endpoints},
      use_secure_connection{config["use_secure_connection"].As<bool>(true)} {}

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
