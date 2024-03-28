#pragma once

#include <string>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief Describes data required for decrypting and
/// authenticating EncryptedPassportElement.
/// @see https://core.telegram.org/bots/api#encryptedcredentials
/// @see https://core.telegram.org/passport#receiving-information for a complete
/// description of the data decryption and authentication processes.
struct EncryptedCredentials {
  /// @brief Base64-encoded encrypted JSON-serialized data with unique
  /// user's payload, data hashes and secrets required for
  /// EncryptedPassportElement decryption and authentication.
  std::string data;

  /// @brief Base64-encoded data hash for data authentication.
  std::string hash;

  /// @brief Base64-encoded secret, encrypted with the bot's public RSA key.
  /// @note Required for data decryption.
  std::string secret;
};

EncryptedCredentials Parse(const formats::json::Value& json,
                           formats::parse::To<EncryptedCredentials>);

formats::json::Value Serialize(const EncryptedCredentials& encrypted_credentials,
                               formats::serialize::To<formats::json::Value>);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
