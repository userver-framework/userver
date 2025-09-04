#include <userver/kafka/impl/broker_secrets.hpp>

#include <userver/formats/parse/common_containers.hpp>
#include <userver/logging/log.hpp>

USERVER_NAMESPACE_BEGIN

namespace kafka::impl {

Secret Parse(const formats::json::Value& doc, formats::parse::To<Secret>) {
    auto brokers = doc["brokers"].As<std::string>();
    if (doc.HasMember("username")) {
        return Secret{
            std::move(brokers),
            Secret::SaslCredentials{
                doc["username"].As<Secret::SecretType>(),
                doc["password"].As<Secret::SecretType>(),
            }};
    } else if (doc.HasMember("ssl_certificate_location")) {
        return Secret{
            std::move(brokers),
            Secret::SslCredentials{
                doc["ssl_certificate_location"].As<Secret::SecretType>(),
                doc["ssl_key_location"].As<Secret::SecretType>(),
                doc["ssl_key_password"].As<std::optional<Secret::SecretType>>(),
            }};
    } else {
        return Secret{std::move(brokers)};
    }
}

BrokerSecrets::BrokerSecrets(const formats::json::Value& doc) {
    if (!doc.HasMember("kafka_settings")) {
        LOG_ERROR("No 'kafka_settings' in secdist");
    }
    secret_by_component_name_ = doc["kafka_settings"].As<std::map<std::string, Secret>>({});
}

const Secret& BrokerSecrets::GetSecretByComponentName(const std::string& component_name) const {
    const auto secret_it = secret_by_component_name_.find(component_name);
    if (secret_it != secret_by_component_name_.end()) {
        return secret_it->second;
    }

    throw std::runtime_error{fmt::format("No secrets for '{}' component", component_name)};
}

}  // namespace kafka::impl

USERVER_NAMESPACE_END
