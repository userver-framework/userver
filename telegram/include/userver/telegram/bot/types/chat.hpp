#pragma once

#include <userver/telegram/bot/types/chat_location.hpp>
#include <userver/telegram/bot/types/chat_permissions.hpp>
#include <userver/telegram/bot/types/chat_photo.hpp>

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>
#include <string>
#include <vector>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

struct Message;

/// @brief This object represents a chat.
/// @see https://core.telegram.org/bots/api#chat
struct Chat {
  /// @brief Unique identifier for this chat.
  std::int64_t id{};

  enum class Type {
    kPrivate, kGroup, kSupergroup, kChannel
  };

  /// @brief Type of chat.
  Type type;

  /// @brief Optional. Title, for supergroups, channels and group chats.
  std::optional<std::string> title;

  /// @brief Optional. Username, for private chats, 
  /// supergroups and channels if available.
  std::optional<std::string> username;

  /// @brief Optional. First name of the other party in a private chat.
  std::optional<std::string> first_name;

  /// @brief Optional. Last name of the other party in a private chat.
  std::optional<std::string> last_name;

  /// @brief Optional. True, if the supergroup chat
  /// is a forum (has topics enabled).
  /// @see https://telegram.org/blog/topics-in-groups-collectible-usernames#topics-in-groups
  std::optional<bool> is_forum;

  /// @brief Optional. Chat photo.
  /// @note Returned only in GetChat.
  std::unique_ptr<ChatPhoto> photo;

  /// @brief Optional. If non-empty, the list of all active chat usernames;
  /// for private chats, supergroups and channels.
  /// @note Returned only in GetChat.
  std::optional<std::vector<std::string>> active_usernames;

  /// @brief Optional. Custom emoji identifier of emoji status of
  /// the other party in a private chat.
  /// @note Returned only in GetChat.
  std::optional<std::string> emoji_status_custom_emoji_id;

  /// @brief Optional. Expiration date of the emoji status of the other
  /// party in a private chat in Unix time, if any.
  /// @note Returned only in GetChat.
  std::optional<std::chrono::system_clock::time_point> emoji_status_expiration_date;

  /// @brief Optional. Bio of the other party in a private chat.
  /// @note Returned only in GetChat.
  std::optional<std::string> bio;

  /// @brief Optional. True, if privacy settings of the other party in the
  /// private chat allows to use tg://user?id=<user_id> links only in chats
  /// with the user.
  /// @note Returned only in GetChat.
  std::optional<bool> has_private_forwards;

  /// @brief Optional. True, if the privacy settings of the other party
  /// restrict sending voice and video note messages in the private chat.
  /// @note Returned only in GetChat.
  std::optional<bool> has_restricted_voice_and_video_messages;

  /// @brief Optional. True, if users need to join the supergroup
  /// before they can send messages.
  /// @note Returned only in GetChat.
  std::optional<bool> join_to_send_messages;

  /// @brief Optional. True, if all users directly joining the
  /// supergroup need to be approved by supergroup administrators.
  /// @note Returned only in GetChat.
  std::optional<bool> join_by_request;

  /// @brief Optional. Description, for groups, supergroups and channel chats.
  /// @note Returned only in GetChat.
  std::optional<std::string> description;

  /// @brief Optional. Primary invite link, for groups, supergroups
  /// and channel chats.
  /// @note Returned only in GetChat.
  std::optional<std::string> invite_link;

  /// @brief Optional. The most recent pinned message (by sending date).
  /// @note Returned only in GetChat.
  std::unique_ptr<Message> pinned_message;

  /// @brief Optional. Default chat member permissions, for groups
  /// and supergroups.
  /// @note Returned only in GetChat.
  std::unique_ptr<ChatPermissions> permissions;

  /// @brief Optional. For supergroups, the minimum allowed delay between
  /// consecutive messages sent by each unpriviledged user; in seconds.
  /// @note Returned only in GetChat.
  std::optional<std::chrono::seconds> slow_mode_delay;

  /// @brief Optional. The time after which all messages sent to the chat
  /// will be automatically deleted; in seconds.
  /// @note Returned only in GetChat.
  std::optional<std::chrono::seconds> message_auto_delete_time;

  /// @brief Optional. True, if aggressive anti-spam checks are enabled
  /// in the supergroup. The field is only available to chat administrators.
  /// @note Returned only in GetChat.
  std::optional<bool> has_aggressive_anti_spam_enabled;

  /// @brief Optional. True, if non-administrators can only get the list
  /// of bots and administrators in the chat.
  /// @note Returned only in GetChat.
  std::optional<bool> has_hidden_members;

  /// @brief Optional. True, if messages from the chat can't be forwarded
  /// to other chats.
  /// @note Returned only in GetChat.
  std::optional<bool> has_protected_content;

  /// @brief Optional. For supergroups, name of group sticker set.
  /// @note Returned only in GetChat.
  std::optional<std::string> sticker_set_name;

  /// @brief Optional. True, if the bot can change the group sticker set.
  /// @note Returned only in GetChat.
  std::optional<bool> can_set_sticker_set;

  /// @brief Optional. Unique identifier for the linked chat.
  /// @note Returned only in GetChat.
  std::optional<std::int64_t> linked_chat_id;

  /// @brief Optional. For supergroups, the location to which the
  /// supergroup is connected.
  /// @note Returned only in GetChat.
  std::unique_ptr<ChatLocation> location;
};

std::string_view ToString(Chat::Type chat_type);

Chat::Type Parse(const formats::json::Value& value,
                 formats::parse::To<Chat::Type>);

formats::json::Value Serialize(Chat::Type chat_type,
                               formats::serialize::To<formats::json::Value>);

Chat Parse(const formats::json::Value& json,
           formats::parse::To<Chat>);

formats::json::Value Serialize(const Chat& chat,
                               formats::serialize::To<formats::json::Value>);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
