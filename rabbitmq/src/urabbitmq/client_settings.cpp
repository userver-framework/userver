#include <userver/urabbitmq/client_settings.hpp>

#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <userver/formats/json/value.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/storages/secdist/helpers.hpp>
#include <userver/utils/assert.hpp>

#include <boost/range/adaptor/map.hpp>

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

}  // namespace

AuthSettings Parse(const formats::json::Value& doc,
                   formats::parse::To<AuthSettings>) {
  AuthSettings auth;
  auth.login = doc["login"].As<std::string>();
  auth.password = doc["password"].As<std::string>();
  auth.vhost = doc["vhost"].As<std::string>();

  TlsSettings tls_settings;
  const auto& client_cert_path = doc["tls"]["client-cert-path"];
  const auto& client_private_key_path = doc["tls"]["client-private-key-path"];
  if (client_cert_path.IsMissing() != client_private_key_path.IsMissing()) {
    throw std::runtime_error(
        "Either set both tls.client-cert-path and "
        "tls.client-private-key-path options or none of "
        "them");
  }
  if (!client_cert_path.IsMissing()) {
    ClientCertSettings client_cert_settings;
    client_cert_settings.cert_path = client_cert_path.As<std::string>();
    client_cert_settings.private_key_path =
        client_private_key_path.As<std::string>();
    tls_settings.client_cert_settings = client_cert_settings;
  }
  tls_settings.ca_cert_paths =
      doc["tls"]["ca-paths"].As<std::vector<std::string>>({});
  tls_settings.verify_host = doc["tls"]["verify_host"].As<bool>(true);

  if (tls_settings.client_cert_settings ||
      !tls_settings.ca_cert_paths.empty() || !tls_settings.verify_host) {
    auth.tls_settings = tls_settings;
  }

  return auth;
}

RabbitEndpoints Parse(const formats::json::Value& doc,
                      formats::parse::To<RabbitEndpoints>) {
  auto port = doc["port"].As<uint16_t>();
  auto hosts = ParseHosts(doc);

  RabbitEndpoints result;
  result.endpoints.reserve(hosts.size());
  for (auto& host : hosts) {
    result.endpoints.push_back({std::move(host), port});
  }
  result.auth = doc.As<AuthSettings>();

  return result;
}

PoolSettings Parse(const yaml_config::YamlConfig& config,
                   formats::parse::To<PoolSettings>) {
  PoolSettings result{};
  result.min_pool_size =
      config["min_pool_size"].As<size_t>(result.min_pool_size);
  result.max_pool_size =
      config["max_pool_size"].As<size_t>(result.max_pool_size);
  result.max_in_flight_requests = config["max_in_flight_requests"].As<size_t>(
      result.max_in_flight_requests);

  UINVARIANT(result.min_pool_size <= result.max_pool_size,
             "max_pool_size is less than min_pool_size");
  UINVARIANT(result.max_pool_size > 0, "max_pool_size is set to zero");

  return result;
}

ClientSettings::ClientSettings() = default;

ClientSettings::ClientSettings(const components::ComponentConfig& config,
                               const RabbitEndpoints& rabbit_endpoints)
    : pool_settings{config.As<PoolSettings>()},
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
    throw std::runtime_error{fmt::format(
        "RMQ broken '{}' is not found in secdist. Available endpoints: [{}]",
        name, fmt::join(endpoints_ | boost::adaptors::map_keys, ", "))};
  }

  return it->second;
}

}  // namespace urabbitmq

USERVER_NAMESPACE_END
