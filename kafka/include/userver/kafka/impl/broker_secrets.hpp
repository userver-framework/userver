#pragma once

#include <map>
#include <optional>
#include <variant>

#include <userver/formats/json/value.hpp>
#include <userver/utils/strong_typedef.hpp>

USERVER_NAMESPACE_BEGIN

namespace kafka::impl {

struct Secret final {
    using SecretType = utils::NonLoggable<class SecretTag, std::string>;

    struct SaslCredentials final {
        /// @brief SASL username for use with the PLAIN and SASL-SCRAM-.. mechanisms.
        SecretType username;
        /// @brief SASL password for use with the PLAIN and SASL-SCRAM-.. mechanisms.
        SecretType password;
    };

    struct SslCredentials final {
        /// @brief Path to client's public key (PEM) used for authentication.
        SecretType ssl_certificate_location;
        /// @brief Path to client's private key (PEM) used for authentication.
        SecretType ssl_key_location;
        /// @brief (Optional) Private key passphrase.
        std::optional<SecretType> ssl_key_password{};
    };

    /// @brief Brokers URI comma-separated list.
    /// @note Is is allowed to pass only one broker URI.
    /// Client discovers over brokers automatically.
    std::string brokers;

    /// @brief Security protocol corresponding credentials.
    /// PLAINTEXT -> none
    /// SASL_PLAINTEXT/SASL_SSL -> SaslCredentials
    /// SSL -> SslCredentials
    std::variant<std::monostate, SaslCredentials, SslCredentials> credentials{};
};

class BrokerSecrets final {
public:
    explicit BrokerSecrets(const formats::json::Value& doc);

    const Secret& GetSecretByComponentName(const std::string& component_name) const;

private:
    std::map<std::string, Secret> secret_by_component_name_;
};

}  // namespace kafka::impl

USERVER_NAMESPACE_END
