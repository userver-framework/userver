#pragma once

#include <userver/telegram/bot/types/chat.hpp>
#include <userver/telegram/bot/types/chat_invite_link.hpp>
#include <userver/telegram/bot/types/user.hpp>

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief Represents a join request sent to a chat.
/// @see https://core.telegram.org/bots/api#chatjoinrequest
struct ChatJoinRequest {
  /// @brief Chat to which the request was sent.
  std::unique_ptr<Chat> chat;

  /// @brief User that sent the join request.
  std::unique_ptr<User> from;

  /// @brief Identifier of a private chat with the user who sent the join
  /// request.
  /// @note The bot can use this identifier for 24 hours to send messages until
  /// the join request is processed, assuming no other administrator
  /// contacted the user.
  std::int64_t user_chat_id{};

  /// @brief Date the request was sent in Unix time.
  std::chrono::system_clock::time_point date{};

  /// @brief Optional. Bio of the user.
  std::optional<std::string> bio;

  /// @brief Optional. Chat invite link that was used by the user to send the
  /// join request.
  std::unique_ptr<ChatInviteLink> invite_link;
};

ChatJoinRequest Parse(const formats::json::Value& json,
                      formats::parse::To<ChatJoinRequest>);

formats::json::Value Serialize(const ChatJoinRequest& chat_join_request,
                               formats::serialize::To<formats::json::Value>);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
