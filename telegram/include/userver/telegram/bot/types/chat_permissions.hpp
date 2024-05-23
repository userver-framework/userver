#pragma once

#include <optional>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief Describes actions that a non-administrator user is allowed
/// to take in a chat.
/// @see https://core.telegram.org/bots/api#chatpermissions
struct ChatPermissions {
  /// @brief Optional. True, if the user is allowed to send text
  /// messages, contacts, invoices, locations and venues.
  std::optional<bool> can_send_messages;

  /// @brief Optional. True, if the user is allowed to send audios.
  std::optional<bool> can_send_audios;

  /// @brief Optional. True, if the user is allowed to send documents.
  std::optional<bool> can_send_documents;

  /// @brief Optional. True, if the user is allowed to send photos.
  std::optional<bool> can_send_photos;

  /// @brief Optional. True, if the user is allowed to send videos.
  std::optional<bool> can_send_videos;

  /// @brief Optional. True, if the user is allowed to send video notes.
  std::optional<bool> can_send_video_notes;

  /// @brief Optional. True, if the user is allowed to send voice notes.
  std::optional<bool> can_send_voice_notes;

  /// @brief Optional. True, if the user is allowed to send polls.
  std::optional<bool> can_send_polls;

  /// @brief Optional. True, if the user is allowed to send animations,
  /// games, stickers and use inline bots.
  std::optional<bool> can_send_other_messages;

  /// @brief Optional. True, if the user is allowed to add web
  /// page previews to their messages.
  std::optional<bool> can_add_web_page_previews;

  /// @brief Optional. True, if the user is allowed to change the chat title,
  /// photo and other settings.
  /// @note Ignored in public supergroups.
  std::optional<bool> can_change_info;

  /// @brief Optional. True, if the user is allowed to invite
  /// new users to the chat.
  std::optional<bool> can_invite_users;

  /// @brief Optional. True, if the user is allowed to pin messages.
  /// @note Ignored in public supergroups.
  std::optional<bool> can_pin_messages;

  /// @brief Optional. True, if the user is allowed to create forum topics.
  /// @note If omitted defaults to the value of can_pin_messages.
  std::optional<bool> can_manage_topics;
};

ChatPermissions Parse(const formats::json::Value& json,
                      formats::parse::To<ChatPermissions>);

formats::json::Value Serialize(const ChatPermissions& chat_permissions,
                               formats::serialize::To<formats::json::Value>);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
