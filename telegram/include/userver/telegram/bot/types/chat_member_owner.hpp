#pragma once

#include <userver/telegram/bot/types/user.hpp>

#include <memory>
#include <optional>
#include <string>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief Represents a chat member that owns the chat and has all
/// administrator privileges.
/// @see https://core.telegram.org/bots/api#chatmemberowner
struct ChatMemberOwner {
  /// @brief The member's status in the chat, always "creator".
  static constexpr std::string_view kStatus = "creator";

  /// @brief Information about the user.
  std::unique_ptr<User> user;

  /// @brief True, if the user's presence in the chat is hidden.
  bool is_anonymous{};

  /// @brief Optional. Custom title for this user.
  std::optional<std::string> custom_title;
};

ChatMemberOwner Parse(const formats::json::Value& json,
                      formats::parse::To<ChatMemberOwner>);

formats::json::Value Serialize(const ChatMemberOwner& chat_member_owner,
                               formats::serialize::To<formats::json::Value>);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
