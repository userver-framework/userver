#pragma once

#include <userver/telegram/bot/types/encrypted_credentials.hpp>
#include <userver/telegram/bot/types/encrypted_passport_element.hpp>

#include <memory>
#include <vector>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief Describes Telegram Passport data shared with the bot by the user.
/// @see https://core.telegram.org/bots/api#passportdata
struct PassportData {
  /// @brief Array with information about documents and other
  /// Telegram Passport elements that was shared with the bot
  std::vector<EncryptedPassportElement> data;

  /// @brief Encrypted credentials required to decrypt the data
  std::unique_ptr<EncryptedCredentials> credentials;
};

PassportData Parse(const formats::json::Value& json,
                   formats::parse::To<PassportData>);

formats::json::Value Serialize(const PassportData& passport_data,
                               formats::serialize::To<formats::json::Value>);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
