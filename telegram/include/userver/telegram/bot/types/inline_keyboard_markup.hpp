#pragma once

#include <userver/telegram/bot/types/inline_keyboard_button.hpp>

#include <vector>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief This object represents an inline keyboard that appears
/// right next to the message it belongs to.
/// @see https://core.telegram.org/bots/api#inlinekeyboardmarkup
struct InlineKeyboardMarkup {
  /// @brief Array of button rows, each represented by an Array of 
  /// InlineKeyboardButton objects.
  std::vector<std::vector<InlineKeyboardButton>> inline_keyboard;
};

InlineKeyboardMarkup Parse(const formats::json::Value& json,
                           formats::parse::To<InlineKeyboardMarkup>);

formats::json::Value Serialize(const InlineKeyboardMarkup& inline_keyboard_markup,
                               formats::serialize::To<formats::json::Value>);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
