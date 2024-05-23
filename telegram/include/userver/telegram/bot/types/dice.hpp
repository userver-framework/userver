#pragma once

#include <cstdint>
#include <string>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief This object represents an animated emoji that displays
/// a random value.
/// @see https://core.telegram.org/bots/api#dice
struct Dice {
  /// @brief Emoji on which the dice throw animation is based.
  std::string emoji;

  /// @brief Value of the dice, 1-6 for “🎲”, “🎯” and “🎳” base emoji,
  /// 1-5 for “🏀” and “⚽” base emoji, 1-64 for “🎰” base emoji.
  std::int64_t value{};
};

Dice Parse(const formats::json::Value& json,
           formats::parse::To<Dice>);

formats::json::Value Serialize(const Dice& dice,
                               formats::serialize::To<formats::json::Value>);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
