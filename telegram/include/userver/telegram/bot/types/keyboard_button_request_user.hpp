#pragma once

#include <cstdint>
#include <optional>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief This object defines the criteria used to request a suitable user.
/// The identifier of the selected user will be shared with the bot when the
/// corresponding button is pressed.
/// @see https://core.telegram.org/bots/api#keyboardbuttonrequestuser
/// @see https://core.telegram.org/bots/features#chat-and-user-selection
struct KeyboardButtonRequestUser {
  /// @brief Signed 32-bit identifier of the request, which will be received
  /// back in the UserShared object.
  /// @note Must be unique within the message.
  std::int64_t request_id{};

  /// @brief Optional. Pass True to request a bot, pass False to request 
  /// a regular user.
  /// @note If not specified, no additional restrictions are applied.
  std::optional<bool> user_is_bot;

  /// @brief Optional. Pass True to request a premium user, pass False
  /// to request a non-premium user.
  /// @note If not specified, no additional restrictions are applied.
  std::optional<bool> user_is_premium;
};

KeyboardButtonRequestUser Parse(const formats::json::Value& json,
                                formats::parse::To<KeyboardButtonRequestUser>);

formats::json::Value Serialize(
    const KeyboardButtonRequestUser& keyboard_button_request_user,
    formats::serialize::To<formats::json::Value>);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
