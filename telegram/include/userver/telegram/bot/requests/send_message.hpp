#pragma once

#include <userver/telegram/bot/requests/request.hpp>
#include <userver/telegram/bot/types/chat_id.hpp>
#include <userver/telegram/bot/types/message_entity.hpp>
#include <userver/telegram/bot/types/message.hpp>
#include <userver/telegram/bot/types/reply_markup.hpp>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief Use this method to send text messages.
/// @see https://core.telegram.org/bots/api#sendmessage
struct SendMessageMethod {
  static constexpr std::string_view kName = "sendMessage";

  static constexpr auto kHttpMethod = clients::http::HttpMethod::kPost;

  struct Parameters{
    Parameters(ChatId _chat_id, std::string _text);

    /// @brief Unique identifier for the target chat or username of the
    /// target channel (in the format @channelusername).
    ChatId chat_id;

    /// @brief Unique identifier for the target message thread (topic)
    // of the forum;
    /// @note For forum supergroups only.
    std::optional<std::int64_t> message_thread_id;

    /// @brief Text of the message to be sent, 1-4096 characters after
    /// entities parsing.
    std::string text;

    /// @brief Mode for parsing entities in the message text.
    /// @see https://core.telegram.org/bots/api#formatting-options for more
    /// details.
    std::optional<std::string> parse_mode;

    /// @brief List of special entities that appear in message text, which can
    /// be specified instead of parse_mode.
    std::optional<std::vector<MessageEntity>> entities;

    /// @brief Disables link previews for links in this message.
    std::optional<bool> disable_web_page_preview;

    /// @brief Sends the message silently. Users will receive a notification
    /// with no sound.
    std::optional<bool> disable_notification;

    /// @brief Protects the contents of the sent message from
    /// forwarding and saving.
    std::optional<bool> protect_content;

    /// @brief If the message is a reply, ID of the original message.
    std::optional<std::int64_t> reply_to_message_id;

    /// @brief Pass True if the message should be sent even if the specified
    /// replied-to message is not found.
    std::optional<bool> allow_sending_without_reply;

    /// @brief Additional interface options.
    /// @see https://core.telegram.org/bots/features#inline-keyboards
    /// @see https://core.telegram.org/bots/features#keyboards
    std::optional<ReplyMarkup> reply_markup;
  };

  /// @brief On success, the sent Message is returned.
  using Reply = Message;

  static void FillRequestData(clients::http::Request& request,
                              const Parameters& parameters);

  static Reply ParseResponseData(clients::http::Response& response);
};

SendMessageMethod::Parameters Parse(
    const formats::json::Value& json,
    formats::parse::To<SendMessageMethod::Parameters>);

formats::json::Value Serialize(const SendMessageMethod::Parameters& parameters,
                               formats::serialize::To<formats::json::Value>);

using SendMessageRequest = Request<SendMessageMethod>;

}  // namespace telegram::bot

USERVER_NAMESPACE_END
