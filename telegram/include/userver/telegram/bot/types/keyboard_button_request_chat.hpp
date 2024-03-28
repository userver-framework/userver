#pragma once

#include <userver/telegram/bot/types/chat_administrator_rights.hpp>

#include <cstdint>
#include <memory>
#include <optional>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief This object defines the criteria used to request a suitable chat.
/// The identifier of the selected chat will be shared with the bot when the
/// corresponding button is pressed.
/// @see https://core.telegram.org/bots/api#keyboardbuttonrequestchat
/// @see https://core.telegram.org/bots/features#chat-and-user-selection
struct KeyboardButtonRequestChat {
  /// @brief Signed 32-bit identifier of the request, which will be received 
  /// back in the ChatShared object.
  /// @note Must be unique within the message.
  std::int64_t request_id{};

  /// @brief Pass True to request a channel chat, pass False to request 
  /// a group or a supergroup chat.
  bool chat_is_channel{};

  /// @brief Optional. Pass True to request a forum supergroup, pass False to
  /// request a non-forum chat.
  /// @note If not specified, no additional restrictions are applied.
  std::optional<bool> chat_is_forum;

  /// @brief Optional. Pass True to request a supergroup or a channel with
  /// a username, pass False to request a chat without a username.
  /// @note If not specified, no additional restrictions are applied.
  std::optional<bool> chat_has_username;

  /// @brief Optional. Pass True to request a chat owned by the user.
  /// Otherwise, no additional restrictions are applied.
  std::optional<bool> chat_is_created;

  /// @brief Optional. Object listing the required administrator rights of the
  /// user in the chat.
  /// @note The rights must be a superset of bot_administrator_rights.
  /// @note If not specified, no additional restrictions are applied.
  std::unique_ptr<ChatAdministratorRights> user_administrator_rights;

  /// @brief Optional. A Object listing the required administrator rights of
  /// the bot in the chat.
  /// @note The rights must be a subset of user_administrator_rights.
  /// @note If not specified, no additional restrictions are applied.
  std::unique_ptr<ChatAdministratorRights> bot_administrator_rights;

  /// @brief Optional. Pass True to request a chat with the bot as a member.
  /// Otherwise, no additional restrictions are applied.
  std::optional<bool> bot_is_member;
};

KeyboardButtonRequestChat Parse(
    const formats::json::Value& json,
    formats::parse::To<KeyboardButtonRequestChat>);

formats::json::Value Serialize(
    const KeyboardButtonRequestChat& keyboard_button_request_chat,
    formats::serialize::To<formats::json::Value>);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
