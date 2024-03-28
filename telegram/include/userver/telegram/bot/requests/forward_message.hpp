#pragma once

#include <userver/telegram/bot/requests/request.hpp>
#include <userver/telegram/bot/types/chat_id.hpp>
#include <userver/telegram/bot/types/message.hpp>

#include <cstdint>
#include <optional>
#include <string>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief Use this method to forward messages of any kind.
/// @note Service messages can't be forwarded.
/// @see https://core.telegram.org/bots/api#forwardmessage
struct ForwardMessageMethod {
  static constexpr std::string_view kName = "forwardMessage";

  static constexpr auto kHttpMethod = clients::http::HttpMethod::kPost;

  struct Parameters{
    Parameters(ChatId _chat_id,
               ChatId _from_chat_id,
               std::int64_t _message_id);

    /// @brief Unique identifier for the target chat or username of the
    /// target channel (in the format @channelusername).
    ChatId chat_id;

    /// @brief Unique identifier for the target message thread (topic) of
    /// the forum.
    /// @note For forum supergroups only.
    std::optional<std::int64_t> message_thread_id;

    /// @brief Unique identifier for the chat where the original message was
    /// sent (or channel username in the format @channelusername)
    ChatId from_chat_id;

    /// @brief Sends the message silently. Users will receive a notification
    /// with no sound.
    std::optional<bool> disable_notification;

    /// @brief Protects the contents of the forwarded message from
    /// forwarding and saving.
    std::optional<bool> protect_content;

    /// @brief Message identifier in the chat specified in from_chat_id.
    std::int64_t message_id{};
  };

  /// @note Service messages can't be forwarded.
  using Reply = Message;

  static void FillRequestData(clients::http::Request& request,
                              const Parameters& parameters);

  static Reply ParseResponseData(clients::http::Response& response);
};

ForwardMessageMethod::Parameters Parse(const formats::json::Value& json,
                                       formats::parse::To<ForwardMessageMethod::Parameters>);

formats::json::Value Serialize(const ForwardMessageMethod::Parameters& parameters,
                               formats::serialize::To<formats::json::Value>);

using ForwardMessageRequest = Request<ForwardMessageMethod>;

}  // namespace telegram::bot

USERVER_NAMESPACE_END
