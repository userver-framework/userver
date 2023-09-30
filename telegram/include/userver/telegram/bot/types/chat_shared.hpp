#pragma once

#include <cstdint>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief This object contains information about the chat whose
/// identifier was shared with the bot using a KeyboardButtonRequestChat button.
/// @see https://core.telegram.org/bots/api#chatshared
/// @see https://core.telegram.org/bots/api#keyboardbuttonrequestchat
struct ChatShared {
  /// @brief Identifier of the request.
  std::int64_t request_id{};

  /// @brief Identifier of the shared chat.
  /// @note The bot may not have access to the chat and could be unable to use
  /// this identifier, unless the chat is already known to the bot by some
  /// other means.
  std::int64_t chat_id{};
};

ChatShared Parse(const formats::json::Value& json,
                 formats::parse::To<ChatShared>);

formats::json::Value Serialize(const ChatShared& chat_shared,
                               formats::serialize::To<formats::json::Value>);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
