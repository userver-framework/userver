#pragma once

#include <optional>
#include <string>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief This object represents an inline button that switches the current
/// user to inline mode in a chosen chat, with an optional default inline query.
/// @see https://core.telegram.org/bots/api#switchinlinequerychosenchat
struct SwitchInlineQueryChosenChat {
  /// @brief Optional. The default inline query to be inserted in the input
  /// field.
  /// @note If left empty, only the bot's username will be inserted.
  std::optional<std::string> query;

  /// @brief Optional. True, if private chats with users can be chosen.
  std::optional<bool> allow_user_chats;

  /// @brief Optional. True, if private chats with bots can be chosen.
  std::optional<bool> allow_bot_chats;

  /// @brief Optional. True, if group and supergroup chats can be chosen.
  std::optional<bool> allow_group_chats;

  /// @brief Optional. True, if channel chats can be chosen.
  std::optional<bool> allow_channel_chats;
};

SwitchInlineQueryChosenChat Parse(const formats::json::Value& json,
                                  formats::parse::To<SwitchInlineQueryChosenChat>);

formats::json::Value Serialize(const SwitchInlineQueryChosenChat& switch_inline_query_chosen_chat,
                               formats::serialize::To<formats::json::Value>);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
