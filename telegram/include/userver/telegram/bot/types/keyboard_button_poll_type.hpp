#pragma once

#include <optional>
#include <string>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief This object represents type of a poll, which is allowed to be
/// created and sent when the corresponding button is pressed.
/// @see https://core.telegram.org/bots/api#keyboardbuttonpolltype
struct KeyboardButtonPollType {
  enum class PoolType {
    kQuiz,     ///< The user is allowed to create only polls in the quiz mode.
    kRegular,  ///< The user is allowed to create only regular polls.
    kAny       ///< The user is allowed to create a poll of any type.
  };
  /// @brief Optional. Type of a poll, which is allowed to be created and sent.
  std::optional<PoolType> type;
};

std::string_view ToString(KeyboardButtonPollType::PoolType poll_type);

KeyboardButtonPollType::PoolType Parse(
    const formats::json::Value& value,
    formats::parse::To<KeyboardButtonPollType::PoolType>);

formats::json::Value Serialize(KeyboardButtonPollType::PoolType poll_type,
                               formats::serialize::To<formats::json::Value>);

KeyboardButtonPollType Parse(const formats::json::Value& json,
                             formats::parse::To<KeyboardButtonPollType>);

formats::json::Value Serialize(
    const KeyboardButtonPollType& keyboard_button_poll_type,
    formats::serialize::To<formats::json::Value>);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
