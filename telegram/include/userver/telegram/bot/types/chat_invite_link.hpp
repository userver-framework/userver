#pragma once

#include <userver/telegram/bot/types/user.hpp>

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief Represents an invite link for a chat.
/// @see https://core.telegram.org/bots/api#chatinvitelink
struct ChatInviteLink {
  /// @brief The invite link.
  /// @note If the link was created by another chat administrator,
  /// then the second part of the link will be replaced with “…”.
  std::string invite_link;

  /// @brief Creator of the link.
  std::unique_ptr<User> creator;

  /// @brief True, if users joining the chat via the link need to be
  /// approved by chat administrators.
  bool creates_join_request{};

  /// @brief True, if the link is primary.
  bool is_primary{};

  /// @brief True, if the link is revoked.
  bool is_revoked{};

  /// @brief Optional. Invite link name.
  std::optional<std::string> name;

  /// @brief Optional. Point in time (Unix timestamp) when the link will
  /// expire or has been expired.
  std::optional<std::chrono::system_clock::time_point> expire_date;

  /// @brief Optional. The maximum number of users that can be members of
  /// the chat simultaneously after joining the chat via this invite link.
  /// 1-99999
  std::optional<std::int64_t> member_limit;

  /// @brief Optional. Number of pending join requests created using this link.
  std::optional<std::int64_t> pending_join_request_count;
};

ChatInviteLink Parse(const formats::json::Value& json,
                     formats::parse::To<ChatInviteLink>);

formats::json::Value Serialize(const ChatInviteLink& chat_invite_link,
                               formats::serialize::To<formats::json::Value>);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
