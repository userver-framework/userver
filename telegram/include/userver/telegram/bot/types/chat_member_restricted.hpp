#pragma once

#include <userver/telegram/bot/types/user.hpp>

#include <chrono>
#include <memory>
#include <string>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief Represents a chat member that is under certain restrictions
/// in the chat.
/// @note Supergroups only.
/// @see https://core.telegram.org/bots/api#chatmemberrestricted
struct ChatMemberRestricted {
  /// @brief The member's status in the chat, always "restricted".
  static constexpr std::string_view kStatus = "restricted";

  /// @brief Information about the user
  std::unique_ptr<User> user;

  /// @brief True, if the user is a member of the chat at the moment
  /// of the request.
  bool is_member{};

  /// @brief True, if the user is allowed to send text messages, contacts,
  /// invoices, locations and venues.
  bool can_send_messages{};

  /// @brief True, if the user is allowed to send audios.
  bool can_send_audios{};

  /// @brief True, if the user is allowed to send documents.
  bool can_send_documents{};

  /// @brief True, if the user is allowed to send photos.
  bool can_send_photos{};

  /// @brief True, if the user is allowed to send videos.
  bool can_send_videos{};

  /// @brief True, if the user is allowed to send video notes.
  bool can_send_video_notes{};

  /// @brief True, if the user is allowed to send voice notes.
  bool can_send_voice_notes{};

  /// @brief True, if the user is allowed to send polls.
  bool can_send_polls{};

  /// @brief True, if the user is allowed to send animations, games,
  /// stickers and use inline bots.
  bool can_send_other_messages{};

  /// @brief True, if the user is allowed to add web page previews to
  /// their messages.
  bool can_add_web_page_previews{};

  /// @brief True, if the user is allowed to change the chat title, photo
  /// and other settings.
  bool can_change_info{};

  /// @brief True, if the user is allowed to invite new users to the chat.
  bool can_invite_users{};

  /// @brief True, if the user is allowed to pin messages.
  bool can_pin_messages{};

  /// @brief True, if the user is allowed to create forum topics.
  bool can_manage_topics{};

  /// @brief Date when restrictions will be lifted for this user. Unix time.
  /// @note If 0, then the user is restricted forever.
  std::chrono::system_clock::time_point until_date{};
};

ChatMemberRestricted Parse(const formats::json::Value& json,
                           formats::parse::To<ChatMemberRestricted>);

formats::json::Value Serialize(const ChatMemberRestricted& chat_member_restricted,
                               formats::serialize::To<formats::json::Value>);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
