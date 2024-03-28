#pragma once

#include <userver/telegram/bot/types/callback_game.hpp>
#include <userver/telegram/bot/types/login_url.hpp>
#include <userver/telegram/bot/types/switch_inline_query_chosen_chat.hpp>
#include <userver/telegram/bot/types/web_app_info.hpp>

#include <memory>
#include <optional>
#include <string>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief This object represents one button of an inline keyboard.
/// @note You must use exactly one of the optional fields.
/// @see https://core.telegram.org/bots/api#inlinekeyboardbutton
struct InlineKeyboardButton {
  /// @brief Label text on the button.
  std::string text;

  /// @brief Optional. HTTP or tg:// URL to be opened when the button is
  /// pressed. Links tg://user?id=<user_id> can be used to mention a user by
  /// their ID without using a username, if this is allowed by their privacy
  /// settings.
  std::optional<std::string> url;

  /// @brief Optional. Data to be sent in a CallbackQuery to the bot when
  /// button is pressed, 1-64 bytes.
  std::optional<std::string> callback_data;

  /// @brief Optional. Description of the Web App that will be launched when
  /// the user presses the button. The Web App will be able to send an
  /// arbitrary message on behalf of the user using the method 
  /// AnswerWebAppQuery.
  /// @note Available only in private chats between a user and the bot.
  std::unique_ptr<WebAppInfo> web_app;

  /// @brief Optional. An HTTPS URL used to automatically authorize the user.
  /// @note Can be used as a replacement for the Telegram Login Widget.
  std::unique_ptr<LoginUrl> login_url;

  /// @brief Optional. If set, pressing the button will prompt the user to
  /// select one of their chats, open that chat and insert the bot's username
  /// and the specified inline query in the input field.
  /// @note May be empty, in which case just the bot's username will be
  /// inserted.
  std::optional<std::string> switch_inline_query;

  /// @brief Optional. If set, pressing the button will insert the bot's
  /// username and the specified inline query in the current chat's input field.
  /// @note  May be empty, in which case only the bot's username will be
  /// inserted.
  /// @details This offers a quick way for the user to open your bot in 
  /// inline mode in the same chat - good for selecting something from
  /// multiple options.
  std::optional<std::string> switch_inline_query_current_chat;

  /// @brief Optional. If set, pressing the button will prompt the user to
  /// select one of their chats of the specified type, open that chat and insert
  /// the bot's username and the specified inline query in the input field.
  std::unique_ptr<SwitchInlineQueryChosenChat> switch_inline_query_chosen_chat;

  /// @brief Optional. Description of the game that will be launched when the
  /// user presses the button.
  /// @note This type of button must always be the first button in the first
  /// row.
  std::unique_ptr<CallbackGame> callback_game;

  /// @brief Optional. Specify True, to send a Pay button.
  /// @note This type of button must always be the first button in the first
  /// row and can only be used in invoice messages.
  std::optional<bool> pay;
};

InlineKeyboardButton Parse(const formats::json::Value& json,
                           formats::parse::To<InlineKeyboardButton>);

formats::json::Value Serialize(const InlineKeyboardButton& inline_keyboard_button,
                               formats::serialize::To<formats::json::Value>);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
