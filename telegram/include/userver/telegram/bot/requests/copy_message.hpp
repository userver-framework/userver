#pragma once

#include <userver/telegram/bot/requests/request.hpp>
#include <userver/telegram/bot/types/chat_id.hpp>
#include <userver/telegram/bot/types/message_entity.hpp>
#include <userver/telegram/bot/types/message_id.hpp>
#include <userver/telegram/bot/types/reply_markup.hpp>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief Use this method to copy messages of any kind.
/// @note Service messages and invoice messages can't be copied.
/// @note A quiz poll can be copied only if the value of the field
/// correct_option_id is known to the bot.
/// @note The method is analogous to the method ForwardMessage, but the copied
/// message doesn't have a link to the original message.
/// @see https://core.telegram.org/bots/api#copymessage
struct CopyMessageMethod {
  static constexpr std::string_view kName = "copyMessage";

  static constexpr auto kHttpMethod = clients::http::HttpMethod::kPost;

  struct Parameters{
    Parameters(ChatId _chat_id,
               ChatId _from_chat_id,
               std::int64_t _message_id);

    /// @brief Unique identifier for the target chat or username of the target
    /// channel (in the format @channelusername).
    ChatId chat_id;

    /// @brief Unique identifier for the target message thread (topic) of
    /// the forum.
    /// @note For forum supergroups only.
    std::optional<std::int64_t> message_thread_id;

    /// @brief Unique identifier for the chat where the original message was
    /// sent (or channel username in the format @channelusername).
    ChatId from_chat_id;

    /// @brief Message identifier in the chat specified in from_chat_id.
    std::int64_t message_id{};

    /// @brief New caption for media, 0-1024 characters after entities parsing.
    /// @note If not specified, the original caption is kept.
    std::optional<std::string> caption;

    /// @brief Mode for parsing entities in the new caption. 
    /// @see https://core.telegram.org/bots/api#formatting-options
    /// for more details.
    std::optional<std::string> parse_mode;

    /// @brief List of special entities that appear in the new caption,
    /// which can be specified instead of parse_mode.
    std::optional<std::vector<MessageEntity>> caption_entities;

    /// @brief Sends the message silently. Users will receive a notification
    /// with no sound.
    std::optional<bool> disable_notification;

    /// @brief Protects the contents of the sent message from forwarding
    /// and saving.
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

  /// @brief Returns the MessageId of the sent message on success.
  using Reply = MessageId;

  static void FillRequestData(clients::http::Request& request,
                              const Parameters& parameters);

  static Reply ParseResponseData(clients::http::Response& response);
};

CopyMessageMethod::Parameters Parse(
    const formats::json::Value& json,
    formats::parse::To<CopyMessageMethod::Parameters>);

formats::json::Value Serialize(const CopyMessageMethod::Parameters& parameters,
                               formats::serialize::To<formats::json::Value>);

using CopyMessageRequest = Request<CopyMessageMethod>;

}  // namespace telegram::bot

USERVER_NAMESPACE_END
