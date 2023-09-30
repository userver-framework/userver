#pragma once

#include <userver/telegram/bot/types/keyboard_button_poll_type.hpp>
#include <userver/telegram/bot/types/keyboard_button_request_chat.hpp>
#include <userver/telegram/bot/types/keyboard_button_request_user.hpp>
#include <userver/telegram/bot/types/web_app_info.hpp>

#include <memory>
#include <optional>
#include <string>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief This object represents one button of the reply keyboard.
/// @note The optional fields web_app, request_user, request_chat,
/// request_contact, request_location, and request_poll are mutually exclusive. 
/// @see https://core.telegram.org/bots/api#keyboardbutton
struct KeyboardButton {
  /// @brief Text of the button. If none of the optional fields are used,
  /// it will be sent as a message when the button is pressed.
  std::string text;

  /// @brief Optional. If specified, pressing the button will open a list of
  /// suitable users. Tapping on any user will send their identifier to the bot
  /// in a “user_shared” service message.
  /// @note Available in private chats only.
  /// @note Option will only work in Telegram versions released after
  /// 3 February, 2023. Older clients will display unsupported message.
  std::unique_ptr<KeyboardButtonRequestUser> request_user;

  /// @brief Optional. If specified, pressing the button will open a
  /// list of suitable chats. Tapping on a chat will send its identifier to
  /// the bot in a “chat_shared” service message.
  /// @note Available in private chats only.
  /// @note Option will only work in Telegram versions released
  /// after 3 February, 2023. Older clients will display unsupported message.
  std::unique_ptr<KeyboardButtonRequestChat> request_chat;

  /// @brief Optional. If True, the user's phone number will be sent as a
  /// contact when the button is pressed.
  /// @note Available in private chats only.
  /// @note Option will only work in Telegram versions released after
  /// 9 April, 2016. Older clients will display unsupported message.
  std::optional<bool> request_contact;

  /// @brief Optional. If True, the user's current location will be sent when
  /// the button is pressed.
  /// @note Available in private chats only.
  /// @note Option will only work in Telegram versions released after
  /// 9 April, 2016. Older clients will display unsupported message.
  std::optional<bool> request_location;

  /// @brief Optional. If specified, the user will be asked to create a poll
  /// and send it to the bot when the button is pressed.
  /// @note Available in private chats only.
  /// @note Option will only work in Telegram versions released after
  /// 23 January, 2020. Older clients will display unsupported message.
  std::unique_ptr<KeyboardButtonPollType> request_poll;

  /// @brief Optional. If specified, the described Web App will be launched
  /// when the button is pressed. The Web App will be able to send a
  /// “web_app_data” service message.
  /// @note Available in private chats only.
  /// @note Option will only work in Telegram versions released after
  /// 16 April, 2022. Older clients will display unsupported message.
  std::unique_ptr<WebAppInfo> web_app;
};

KeyboardButton Parse(const formats::json::Value& json,
                     formats::parse::To<KeyboardButton>);

formats::json::Value Serialize(const KeyboardButton& keyboard_button,
                               formats::serialize::To<formats::json::Value>);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
