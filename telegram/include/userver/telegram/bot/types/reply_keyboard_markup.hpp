#pragma once

#include <userver/telegram/bot/types/keyboard_button.hpp>

#include <optional>
#include <string>
#include <vector>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief This object represents a custom keyboard with reply options.
/// @see https://core.telegram.org/bots/api#replykeyboardmarkup
/// @see https://core.telegram.org/bots/features#keyboards for details and
/// examples.
struct ReplyKeyboardMarkup {
  /// @brief Array of button rows, each represented by an Array of
  /// KeyboardButton objects
  std::vector<std::vector<KeyboardButton>> keyboard;

  /// @brief Optional. Requests clients to always show the keyboard when the
  /// regular keyboard is hidden. 
  /// @note Defaults to false, in which case the custom keyboard can be hidden
  /// and opened with a keyboard icon.
  std::optional<bool> is_persistent;

  /// @brief Optional. Requests clients to resize the keyboard vertically for
  /// optimal fit (e.g., make the keyboard smaller if there are just two rows 
  /// of buttons).
  /// @note Defaults to false, in which case the custom keyboard is always of
  /// the same height as the app's standard keyboard.
  std::optional<bool> resize_keyboard;

  /// @brief Optional. Requests clients to hide the keyboard as soon as it's
  /// been used. The keyboard will still be available, but clients will
  /// automatically display the usual letter-keyboard in the chat - the user
  /// can press a special button in the input field to see the custom
  /// keyboard again.
  /// @note Defaults to false.
  std::optional<bool> one_time_keyboard;

  /// @brief Optional. The placeholder to be shown in the input field when the
  /// keyboard is active; 1-64 characters
  std::optional<std::string> input_field_placeholder;

  /// @brief Optional. Use this parameter if you want to show the keyboard to
  /// specific users only. Targets:
  /// 1) users that are @mentioned in the text of the Message object;
  /// 2) if the bot's message is a reply (has reply_to_message_id), sender of
  /// the original message.
  /// @example A user requests to change the bot's language, bot replies to the
  /// request with a keyboard to select the new language. Other users in the
  /// group don't see the keyboard.
  std::optional<bool> selective;
};

ReplyKeyboardMarkup Parse(const formats::json::Value& json,
                          formats::parse::To<ReplyKeyboardMarkup>);

formats::json::Value Serialize(const ReplyKeyboardMarkup& reply_keyboard_markup,
                               formats::serialize::To<formats::json::Value>);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
