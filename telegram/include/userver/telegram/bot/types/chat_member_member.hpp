#pragma once

#include <userver/telegram/bot/types/user.hpp>

#include <memory>
#include <string>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief Represents a chat member that has no additional privileges
/// or restrictions.
/// @see https://core.telegram.org/bots/api#chatmembermember
struct ChatMemberMember {
  /// @brief The member's status in the chat, always "member".
  static constexpr std::string_view kStatus = "member";

  /// @brief Information about the user.
  std::unique_ptr<User> user;
};

ChatMemberMember Parse(const formats::json::Value& json,
                       formats::parse::To<ChatMemberMember>);

formats::json::Value Serialize(const ChatMemberMember& chat_member_member,
                               formats::serialize::To<formats::json::Value>);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
