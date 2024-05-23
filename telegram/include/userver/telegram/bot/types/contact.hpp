#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief This object represents a phone contact.
/// @see https://core.telegram.org/bots/api#contact
struct Contact {
  /// @brief Contact's phone number.
  std::string phone_number;

  /// @brief Contact's first name.
  std::string first_name;

  /// @brief Optional. Contact's last name.
  std::optional<std::string> last_name;

  /// @brief Optional. Contact's user identifier in Telegram.
  std::optional<std::int64_t> user_id;

  /// @brief Optional. Additional data about the contact in the form of a vCard.
  /// @see https://en.wikipedia.org/wiki/VCard
  std::optional<std::string> vcard;
};

Contact Parse(const formats::json::Value& json,
              formats::parse::To<Contact>);

formats::json::Value Serialize(const Contact& contact,
                               formats::serialize::To<formats::json::Value>);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
