#pragma once

#include <userver/telegram/bot/requests/request.hpp>
#include <userver/telegram/bot/types/chat_id.hpp>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief Use this method when you need to tell the user that something is
/// happening on the bot's side.
/// @note The status is set for 5 seconds or less (when a message arrives
/// from your bot, Telegram clients clear its typing status).
/// @example The ImageBot (https://t.me/imagebot) needs some time to process
/// a request and upload the image. Instead of sending a text message along the
/// lines of “Retrieving image, please wait…”, the bot may use sendChatAction
/// with action = Action::kUploadPhoto. The user will see a "sending photo"
/// status for the bot.
/// @note We only recommend using this method when a response from the bot will
/// take a noticeable amount of time to arrive.
/// @see https://core.telegram.org/bots/api#sendchataction
struct SendChatActionMethod {
  static constexpr std::string_view kName = "sendChatAction";

  static constexpr auto kHttpMethod = clients::http::HttpMethod::kPost;

  struct Parameters{
    /// @brief Type of action to broadcast.
    enum class Action {
      kTyping,          ///< For text messages
      kUploadPhoto,     ///< For photos
      kRecordVideo,     ///< For videos
      kUploadVideo,     ///< For videos
      kRecordVoice,     ///< For voice notes
      kUploadVoice,     ///< For voice notes
      kUploadDocument,  ///< For general file
      kChooseSticker,   ///< For stickers
      kFindLocation,    ///< For location data
      kRecordVideoNote, ///< For video notes
      kUploadVideoNote  ///< For video notes
    };

    Parameters(ChatId _chat_id, Action _action);

    /// @brief Unique identifier for the target chat or username of the
    /// target channel (in the format @channelusername).
    ChatId chat_id;

    /// @brief Unique identifier for the target message thread.
    /// @note For supergroups only
    std::optional<std::int64_t> message_thread_id;

    /// @brief Type of action to broadcast.
    Action action;
  };

  /// @brief Returns True on success.
  using Reply = bool;

  static void FillRequestData(clients::http::Request& request,
                              const Parameters& parameters);

  static Reply ParseResponseData(clients::http::Response& response);
};

std::string_view ToString(SendChatActionMethod::Parameters::Action action);

SendChatActionMethod::Parameters::Action Parse(
    const formats::json::Value& value,
    formats::parse::To<SendChatActionMethod::Parameters::Action>);

formats::json::Value Serialize(
    SendChatActionMethod::Parameters::Action action,
    formats::serialize::To<formats::json::Value>);

SendChatActionMethod::Parameters Parse(
    const formats::json::Value& json,
    formats::parse::To<SendChatActionMethod::Parameters>);

formats::json::Value Serialize(
    const SendChatActionMethod::Parameters& parameters,
    formats::serialize::To<formats::json::Value>);

using SendChatActionRequest = Request<SendChatActionMethod>;

}  // namespace telegram::bot

USERVER_NAMESPACE_END
