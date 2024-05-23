#pragma once

#include <userver/telegram/bot/requests/request.hpp>
#include <userver/telegram/bot/types/chat_id.hpp>
#include <userver/telegram/bot/types/input_file.hpp>
#include <userver/telegram/bot/types/message.hpp>
#include <userver/telegram/bot/types/reply_markup.hpp>

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief Use this method to send video messages.
/// @see https://core.telegram.org/bots/api#sendvideonote
struct SendVideoNoteMethod {
  static constexpr std::string_view kName = "sendVideoNote";

  static constexpr auto kHttpMethod = clients::http::HttpMethod::kPost;

  struct Parameters{
    using VideoNote = std::variant<InputFile, std::string>;

    Parameters(ChatId _chat_id, VideoNote _video_note);

    /// @brief Unique identifier for the target chat or username of the target
    /// channel (in the format @channelusername).
    ChatId chat_id;

    /// @brief Unique identifier for the target message thread (topic) of
    /// the forum.
    /// @note For forum supergroups only.
    std::optional<std::int64_t> message_thread_id;

    /// @brief Video note to send.
    /// 1. Pass a file_id as std::string to send a video note that exists on the
    /// Telegram servers (recommended)
    /// 2. Pass InputFile to upload a new video note.
    /// @note Sending video notes by a URL is currently unsupported.
    /// @note As of v.4.0, Telegram clients support rounded square MPEG4 videos
    /// of up to 1 minute long
    /// @see https://core.telegram.org/bots/api#sending-files
    VideoNote video_note;

    /// @brief Duration of sent video in seconds
    std::optional<std::chrono::seconds> duration;

    /// @brief Video width and height, i.e. diameter of the video message
    std::optional<std::int64_t> length;

    /// @brief Thumbnail of the file sent;
    /// @note Can be ignored if thumbnail generation for the file is supported
    /// server-side.
    /// @note The thumbnail should be in JPEG format and less than 200 kB in
    /// size. A thumbnail's width and height should not exceed 320. 
    /// @note Thumbnails can't be reused and can be only uploaded as a new file.
    /// @see https://core.telegram.org/bots/api#sending-files
    std::unique_ptr<InputFile> thumbnail;

    /// @brief Sends the message silently. Users will receive a notification
    /// with no sound.
    std::optional<bool> disable_notification;

    /// @brief Protects the contents of the sent message from forwarding and
    /// saving.
    std::optional<bool> protect_content;

    /// @brief If the message is a reply, ID of the original message
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

SendVideoNoteMethod::Parameters Parse(
    const formats::json::Value& json,
    formats::parse::To<SendVideoNoteMethod::Parameters>);

formats::json::Value Serialize(
    const SendVideoNoteMethod::Parameters& parameters,
    formats::serialize::To<formats::json::Value>);

using SendVideoNoteRequest = Request<SendVideoNoteMethod>;

}  // namespace telegram::bot

USERVER_NAMESPACE_END
