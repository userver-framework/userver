#pragma once

#include <userver/telegram/bot/types/message.hpp>
#include <userver/telegram/bot/types/user.hpp>

#include <memory>
#include <optional>
#include <string>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief This object represents an incoming callback query from a callback
/// button in an inline keyboard.
/// @note If the button that originated the query was attached to a message
/// sent by the bot, the field message will be present.
/// @note If the button was attached to a message sent via the bot
/// (in inline mode), the field inline_message_id will be present.
/// @note Exactly one of the fields data or game_short_name will be present.
/// @note After the user presses a callback button, Telegram clients will
/// display a progress bar until you call AnswerCallbackQuery. It is, therefore,
/// necessary to react by calling AnswerCallbackQuery even if no notification to
/// the user is needed (e.g., without specifying any of the optional
/// parameters).
/// @see https://core.telegram.org/bots/api#callbackquery
struct CallbackQuery {
  /// @brief Unique identifier for this query.
  std::string id;

  /// @brief Sender.
  std::unique_ptr<User> from;

  /// @brief Optional. Message with the callback button that originated
  /// the query.
  /// @note Message content and message date will not be available if
  /// the message is too old.
  std::unique_ptr<Message> message;

  /// @brief Optional. Identifier of the message sent via the bot in inline
  /// mode, that originated the query.
  std::optional<std::string> inline_message_id;

  /// @brief Global identifier, uniquely corresponding to the chat to which
  /// the message with the callback button was sent.
  /// @note Useful for high scores in games (https://core.telegram.org/bots/api#games)
  std::string chat_instance;

  /// @brief Optional. Data associated with the callback button.
  /// @note Be aware that the message originated the query can contain no
  /// callback buttons with this data.
  std::optional<std::string> data;

  /// @brief Optional. Short name of a Game to be returned, serves as the
  /// unique identifier for the game
  std::optional<std::string> game_short_name;
};

CallbackQuery Parse(const formats::json::Value& json,
                    formats::parse::To<CallbackQuery>);

formats::json::Value Serialize(const CallbackQuery& callback_query,
                               formats::serialize::To<formats::json::Value>);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
