#pragma once

#include <userver/telegram/bot/requests/request.hpp>
#include <userver/telegram/bot/types/chat_id.hpp>
#include <userver/telegram/bot/types/input_file.hpp>
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

/// @brief Use this method to send photos. On success, the sent Message is returned.
/// @see https://core.telegram.org/bots/api#sendphoto
struct SendPhotoMethod {
  static constexpr std::string_view kName = "sendPhoto";

  static constexpr auto kHttpMethod = clients::http::HttpMethod::kPost;

  struct Parameters{
    using Photo = std::variant<InputFile, std::string>;

    Parameters(ChatId _chat_id, Photo _photo);

    /// @brief Unique identifier for the target chat or username of the target
    /// channel (in the format @channelusername).
    ChatId chat_id;

    /// @brief Unique identifier for the target message thread (topic) of
    /// the forum.
    /// @note For forum supergroups only.
    std::optional<std::int64_t> message_thread_id;

    /// @brief Photo to send.
    /// 1. Pass a file_id as std::string to send a photo that exists on the
    /// Telegram servers (recommended)
    /// 2. Pass an HTTP URL as a std::string to get a photo from the Internet.
    /// 3. Pass InputFile to upload a new photo.
    /// @note The photo must be at most 10 MB in size. The photo's width and
    /// height must not exceed 10000 in total. Width and height ratio must be
    /// at most 20.
    /// @see https://core.telegram.org/bots/api#sending-files
    Photo photo;

    /// @brief Photo caption (may also be used when resending photos by
    /// file_id), 0-1024 characters after entities parsing
    std::optional<std::string> caption;

    /// @brief Mode for parsing entities in the photo caption.
    /// @see https://core.telegram.org/bots/api#formatting-options
    std::optional<std::string> parse_mode;

    /// @brief List of special entities that appear in the caption, which can
    /// be specified instead of parse_mode.
    std::optional<std::vector<MessageEntity>> caption_entities;

    /// @brief Pass True if the photo needs to be covered with a spoiler
    /// animation.
    std::optional<bool> has_spoiler;

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

  using Reply = Message;

  static void FillRequestData(clients::http::Request& request,
                              const Parameters& parameters);

  static Reply ParseResponseData(clients::http::Response& response);
};

SendPhotoMethod::Parameters Parse(
    const formats::json::Value& json,
    formats::parse::To<SendPhotoMethod::Parameters> to);

formats::json::Value Serialize(
    const SendPhotoMethod::Parameters& parameters,
    formats::serialize::To<formats::json::Value> to);

using SendPhotoRequest = Request<SendPhotoMethod>;

}  // namespace telegram::bot

USERVER_NAMESPACE_END
