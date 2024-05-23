#pragma once

#include <userver/telegram/bot/types/chat.hpp>
#include <userver/telegram/bot/types/chat_invite_link.hpp>
#include <userver/telegram/bot/types/chat_member.hpp>
#include <userver/telegram/bot/types/user.hpp>

#include <chrono>
#include <memory>
#include <optional>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief This object represents changes in the status of a chat member.
/// @see https://core.telegram.org/bots/api#chatmemberupdated
struct ChatMemberUpdated {
  /// @brief Chat the user belongs to.
  std::unique_ptr<Chat> chat;

  /// @brief Performer of the action, which resulted in the change.
  std::unique_ptr<User> from;

  /// @brief Date the change was done in Unix time.
  std::chrono::system_clock::time_point date{};

  /// @brief Previous information about the chat member.
  ChatMember old_chat_member;

  /// @brief New information about the chat member.
  ChatMember new_chat_member;

  /// @brief Optional. Chat invite link, which was used by the user to
  /// join the chat.
  /// @note For joining by invite link events only.
  std::unique_ptr<ChatInviteLink> invite_link;

  /// @brief Optional. True, if the user joined the chat via a chat folder
  /// invite link.
  std::optional<bool> via_chat_folder_invite_link;
};

ChatMemberUpdated Parse(const formats::json::Value& json,
                        formats::parse::To<ChatMemberUpdated>);

formats::json::Value Serialize(const ChatMemberUpdated& chat_member_updated,
                               formats::serialize::To<formats::json::Value>);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
