#include <kafka/impl/broker_secrets.hpp>

#include <userver/formats/parse/common_containers.hpp>
#include <userver/logging/log.hpp>

#include <fmt/format.h>

USERVER_NAMESPACE_BEGIN

namespace kafka::impl {

Secret Parse(const formats::json::Value& doc, formats::parse::To<Secret>) {
  Secret secret{doc["brokers"].As<std::string>(),
                doc["username"].As<std::string>(),
                doc["password"].As<Secret::Password>()};

  return secret;
}

BrokerSecrets::BrokerSecrets(const formats::json::Value& doc) {
  if (!doc.HasMember("kafka_settings")) {
    LOG_ERROR() << "No 'kafka_settings' in secdist";
  }
  secret_by_component_name_ =
      doc["kafka_settings"].As<std::map<std::string, Secret>>();
}

const Secret& BrokerSecrets::GetSecretByComponentName(
    const std::string& component_name) const {
  const auto secret_it = secret_by_component_name_.find(component_name);
  if (secret_it != secret_by_component_name_.end()) {
    return secret_it->second;
  }

  throw std::runtime_error{
      fmt::format("No secrets for {} component", component_name)};
}

}  // namespace kafka::impl

USERVER_NAMESPACE_END
