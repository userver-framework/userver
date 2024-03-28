#pragma once

#include <userver/telegram/bot/types/location.hpp>
#include <userver/telegram/bot/types/user.hpp>

#include <memory>
#include <optional>
#include <string>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief Represents a result of an inline query that was chosen by the user
/// and sent to their chat partner.
/// @note It is necessary to enable inline feedback via @BotFather in order to
/// receive these objects in updates.
/// @see https://core.telegram.org/bots/api#choseninlineresult
struct ChosenInlineResult {
  /// @brief The unique identifier for the result that was chosen.
  std::string result_id;

  /// @brief The user that chose the result.
  std::unique_ptr<User> from;

  /// @brief Optional. Sender location, only for bots that require user
  /// location.
  std::unique_ptr<Location> location;

  /// @brief Optional. Identifier of the sent inline message.
  /// @note Available only if there is an inline keyboard attached
  /// to the message.
  /// @note Will be also received in callback queries and can be used to
  /// edit the message.
  std::optional<std::string> inline_message_id;

  /// @brief The query that was used to obtain the result.
  std::string query;
};

ChosenInlineResult Parse(const formats::json::Value& json,
                         formats::parse::To<ChosenInlineResult>);

formats::json::Value Serialize(const ChosenInlineResult& chosen_inline_result,
                               formats::serialize::To<formats::json::Value>);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
