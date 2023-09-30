#pragma once

#include <optional>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief Upon receiving a message with this object, Telegram clients will
/// remove the current custom keyboard and display the default letter-keyboard.
/// @note By default, custom keyboards are displayed until a new keyboard is
/// sent by a bot. An exception is made for one-time keyboards that are hidden
/// immediately after the user presses a button (see ReplyKeyboardMarkup).
/// @see https://core.telegram.org/bots/api#replykeyboardremove
struct ReplyKeyboardRemove {
  /// @brief Requests clients to remove the custom keyboard (user will not be
  /// able to summon this keyboard)
  /// @note If you want to hide the keyboard from sight
  /// but keep it accessible, use one_time_keyboard in ReplyKeyboardMarkup
  bool remove_keyboard{};

  /// @brief Optional. Use this parameter if you want to remove the keyboard
  /// for specific users only.
  /// Targets:
  /// 1) users that are @mentioned in the text of the see Message object;
  /// 2) if the bot's message is a reply (has reply_to_message_id), sender 
  /// of the original message.
  /// @example A user votes in a poll, bot returns confirmation message in
  /// reply to the vote and removes the keyboard for that user, while still
  /// showing the keyboard with poll options to users who haven't voted yet.
  std::optional<bool> selective;
};

ReplyKeyboardRemove Parse(const formats::json::Value& json,
                          formats::parse::To<ReplyKeyboardRemove>);

formats::json::Value Serialize(const ReplyKeyboardRemove& reply_keyboard_remove,
                               formats::serialize::To<formats::json::Value>);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
