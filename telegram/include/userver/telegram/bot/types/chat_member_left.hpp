#pragma once

#include <userver/telegram/bot/types/user.hpp>

#include <memory>
#include <string>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief Represents a chat member that isn't currently a member of the chat,
/// but may join it themselves.
/// @see https://core.telegram.org/bots/api#chatmemberleft
struct ChatMemberLeft {
  /// @brief The member's status in the chat, always "left".
  static constexpr std::string_view kStatus = "left";

  /// @brief Information about the user.
  std::unique_ptr<User> user;
};

ChatMemberLeft Parse(const formats::json::Value& json,
                     formats::parse::To<ChatMemberLeft>);

formats::json::Value Serialize(const ChatMemberLeft& chat_member_left,
                               formats::serialize::To<formats::json::Value>);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
