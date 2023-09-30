#pragma once

#include <userver/telegram/bot/types/user.hpp>

#include <chrono>
#include <memory>
#include <string>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief Represents a chat member that was banned in the chat and can't
/// return to the chat or view chat messages.
/// @see https://core.telegram.org/bots/api#chatmemberbanned
struct ChatMemberBanned {
  /// @brief The member's status in the chat, always "kicked".
  static constexpr std::string_view kStatus = "kicked";

  /// @brief Information about the user.
  std::unique_ptr<User> user;

  /// @brief Date when restrictions will be lifted for this user. Unix time.
  /// @note If 0, then the user is banned forever.
  std::chrono::system_clock::time_point until_date{};
};

ChatMemberBanned Parse(const formats::json::Value& json,
                       formats::parse::To<ChatMemberBanned>);

formats::json::Value Serialize(const ChatMemberBanned& chat_member_banned,
                               formats::serialize::To<formats::json::Value>);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
