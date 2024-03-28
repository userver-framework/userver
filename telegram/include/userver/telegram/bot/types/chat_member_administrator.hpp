#pragma once

#include <userver/telegram/bot/types/user.hpp>

#include <memory>
#include <optional>
#include <string>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief Represents a chat member that has some additional privileges.
/// @see https://core.telegram.org/bots/api#chatmemberadministrator
struct ChatMemberAdministrator {
  /// @brief The member's status in the chat, always "administrator".
  static constexpr std::string_view kStatus = "administator";

  /// @brief Information about the user.
  std::unique_ptr<User> user;

  /// @brief True, if the bot is allowed to edit administrator privileges of
  /// that user.
  bool can_be_edited{};

  /// @brief True, if the user's presence in the chat is hidden
  bool is_anonymous{};

  /// @brief True, if the administrator can access the chat event log,
  /// chat statistics, boost list in channels, message statistics in channels,
  /// see channel members, see anonymous administrators in supergroups and
  /// ignore slow mode. Implied by any other administrator privilege.
  bool can_manage_chat{};

  /// @brief True, if the administrator can delete messages of other users.
  bool can_delete_messages{};

  /// @brief True, if the administrator can manage video chats.
  bool can_manage_video_chats{};

  /// @brief True, if the administrator can restrict, ban or unban chat members.
  bool can_restrict_members{};

  /// @brief True, if the administrator can add new administrators with a subset
  /// of their own privileges or demote administrators that they have
  /// promoted, directly or indirectly (promoted by administrators that were
  /// appointed by the user).
  bool can_promote_members{};

  /// @brief True, if the user is allowed to change the chat title, photo and
  /// other settings.
  bool can_change_info{};

  /// @brief True, if the user is allowed to invite new users to the chat.
  bool can_invite_users{};

  /// @brief Optional. True, if the administrator can post messages in
  /// the channel.
  /// @note Channels only
  std::optional<bool> can_post_messages;

  /// @brief Optional. True, if the administrator can edit messages of other
  /// users and can pin messages.
  /// @note Channels only.
  std::optional<bool> can_edit_messages;

  /// @brief Optional. True, if the user is allowed to pin messages.
  /// @note Groups and supergroups only.
  std::optional<bool> can_pin_messages;

  /// @brief Optional. True, if the administrator can post stories in
  /// the channel.
  /// @note Channels only.
  std::optional<bool> can_post_stories;

  /// @brief Optional. True, if the administrator can edit stories posted by
  /// other users.
  /// @note Channels only.
  std::optional<bool> can_edit_stories;

  /// @brief Optional. True, if the administrator can delete stories posted by
  /// other users.
  /// @note Channels only.
  std::optional<bool> can_delete_stories;

  /// @brief Optional. True, if the user is allowed to create, rename, close,
  /// and reopen forum topics.
  /// @note Supergroups only
  std::optional<bool> can_manage_topics;

  /// @brief Optional. Custom title for this user.
  std::optional<std::string> custom_title;
};

ChatMemberAdministrator Parse(const formats::json::Value& json,
                              formats::parse::To<ChatMemberAdministrator>);

formats::json::Value Serialize(const ChatMemberAdministrator& chat_member_administrator,
                               formats::serialize::To<formats::json::Value>);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
