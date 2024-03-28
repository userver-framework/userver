#pragma once

#include <optional>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief Represents the rights of an administrator in a chat.
/// @see https://core.telegram.org/bots/api#chatadministratorrights
struct ChatAdministratorRights {
  /// @brief True, if the user's presence in the chat is hidden.
  bool is_anonymous{};

  /// @brief True, if the administrator can access the chat event log, chat
  /// statistics, boost list in channels, message statistics in channels, see
  /// channel members, see anonymous administrators in supergroups and ignore
  /// slow mode. 
  /// @note Implied by any other administrator privilege.
  bool can_manage_chat{};

  /// @brief True, if the administrator can delete messages of other users.
  bool can_delete_messages{};

  /// @brief True, if the administrator can manage video chats.
  bool can_manage_video_chats{};

  /// @brief True, if the administrator can restrict, ban or unban chat members.
  bool can_restrict_members{};

  /// @brief True, if the administrator can add new administrators with a
  /// subset of their own privileges or demote administrators that they have
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
  /// @note Channels only.
  std::optional<bool> can_post_messages;

  /// @brief Optional. True, if the administrator can edit messages of other
  /// users and can pin messages;
  /// @note Channels only.
  std::optional<bool> can_edit_messages;

  /// @brief Optional. True, if the user is allowed to pin messages.
  /// @note Groups and supergroups only
  std::optional<bool> can_pin_messages;

  /// @brief Optional. True, if the administrator can post stories in the
  /// channel.
  /// @note Channels only
  std::optional<bool> can_post_stories;

  /// @brief Optional. True, if the administrator can edit stories posted by
  /// other users.
  /// @note Channels only.
  std::optional<bool> can_edit_stories;

  /// @brief Optional. True, if the administrator can delete stories posted
  /// by other users.
  /// @note Channels only.
  std::optional<bool> can_delete_stories;

  /// @brief Optional. True, if the user is allowed to create, rename, close,
  /// and reopen forum topics.
  /// @note Supergroups only.
  std::optional<bool> can_manage_topics;
};

ChatAdministratorRights Parse(const formats::json::Value& json,
                              formats::parse::To<ChatAdministratorRights>);

formats::json::Value Serialize(const ChatAdministratorRights& chat_administrator_rights,
                               formats::serialize::To<formats::json::Value>);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
