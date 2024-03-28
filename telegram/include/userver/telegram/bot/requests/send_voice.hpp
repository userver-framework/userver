#pragma once

#include <userver/telegram/bot/requests/request.hpp>
#include <userver/telegram/bot/types/chat_id.hpp>
#include <userver/telegram/bot/types/input_file.hpp>
#include <userver/telegram/bot/types/message_entity.hpp>
#include <userver/telegram/bot/types/message.hpp>
#include <userver/telegram/bot/types/reply_markup.hpp>

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief Use this method to send audio files, if you want Telegram clients
/// to display the file as a playable voice message.
/// @see https://core.telegram.org/bots/api#sendvoice
struct SendVoiceMethod {
  static constexpr std::string_view kName = "sendVoice";

  static constexpr auto kHttpMethod = clients::http::HttpMethod::kPost;

  struct Parameters{
    using Voice = std::variant<InputFile, std::string>;
  
    Parameters(ChatId _chat_id, Voice _voice);

    /// @brief Unique identifier for the target chat or username of the
    /// target channel (in the format @channelusername).
    ChatId chat_id;

    /// @brief Unique identifier for the target message thread (topic) of
    /// the forum. 
    /// @note For forum supergroups only.
    std::optional<std::int64_t> message_thread_id;

    /// @brief Voice to send.
    /// 1. Pass a file_id as std::string to send a voice that exists on the
    /// Telegram servers (recommended)
    /// 2. Pass an HTTP URL as a std::string to get a voice from the
    /// Internet.
    /// 3. Pass InputFile to upload a voice file.
    /// @note For this to work, your audio must be in an .OGG file encoded
    /// with OPUS (other formats may be sent as Audio or Document).
    /// @note Bots can currently send voice messages of up to 50 MB in size,
    /// this limit may be changed in the future.
    /// @see https://core.telegram.org/bots/api#sending-files
    Voice voice;

    /// @brief Voice message caption, 0-1024 characters after entities parsing.
    std::optional<std::string> caption;

    /// @brief Mode for parsing entities in the animation caption.
    /// @see https://core.telegram.org/bots/api#formatting-options
    /// for more details.
    std::optional<std::string> parse_mode;

    /// @brief List of special entities that appear in the caption, which can
    /// be specified instead of parse_mode.
    std::optional<std::vector<MessageEntity>> caption_entities;

    /// @brief Duration of the voice message in seconds.
    std::optional<std::chrono::seconds> duration;

    /// @brief Sends the message silently. Users will receive a notification
    /// with no sound.
    std::optional<bool> disable_notification;

    /// @brief Protects the contents of the sent message from forwarding and
    /// saving.
    std::optional<bool> protect_content;

    /// @brief If the message is a reply, ID of the original message.
    std::optional<std::int64_t> reply_to_message_id;

    /// @brief Pass True if the message should be sent even if the specified
    /// replied-to message is not found.
    std::optional<bool> allow_sending_without_reply;

    /// @brief Additional interface options.
    std::optional<ReplyMarkup> reply_markup;
  };

  /// @brief On success, the sent Message is returned.
  using Reply = Message;

  static void FillRequestData(clients::http::Request& request,
                              const Parameters& parameters);

  static Reply ParseResponseData(clients::http::Response& response);
};

SendVoiceMethod::Parameters Parse(
    const formats::json::Value& json,
    formats::parse::To<SendVoiceMethod::Parameters>);

formats::json::Value Serialize(const SendVoiceMethod::Parameters& parameters,
                               formats::serialize::To<formats::json::Value>);

using SendVoiceRequest = Request<SendVoiceMethod>;

}  // namespace telegram::bot

USERVER_NAMESPACE_END
