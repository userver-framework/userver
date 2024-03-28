#pragma once

#include <userver/telegram/bot/requests/request.hpp>
#include <userver/telegram/bot/types/chat_id.hpp>
#include <userver/telegram/bot/types/message.hpp>
#include <userver/telegram/bot/types/reply_markup.hpp>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief Use this method to send an animated emoji that will display a
/// random value.
/// @see https://core.telegram.org/bots/api#senddice
struct SendDiceMethod {
  static constexpr std::string_view kName = "sendDice";

  static constexpr auto kHttpMethod = clients::http::HttpMethod::kPost;

  struct Parameters{
    Parameters(ChatId _chat_id);

    /// @brief Unique identifier for the target chat or username of the
    /// target channel (in the format @channelusername).
    ChatId chat_id;

    /// @brief Unique identifier for the target message thread (topic) of
    /// the forum.
    /// @note For forum supergroups only.
    std::optional<std::int64_t> message_thread_id;

    /// @brief Emoji on which the dice throw animation is based.
    /// @note Currently, must be one of “🎲”, “🎯”, “🏀”, “⚽”, “🎳”, or “🎰”. 
    /// Dice can have values 1-6 for “🎲”, “🎯” and “🎳”, values 1-5 for “🏀”
    /// and “⚽”, and values 1-64 for “🎰”.
    /// @note Defaults to “🎲”.
    std::optional<std::string> emoji;

    /// @brief Sends the message silently. Users will receive a notification
    /// with no sound.
    std::optional<bool> disable_notification;

    /// @brief Protects the contents of the sent message from forwarding.
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

  /// @note On success, the sent Message is returned.
  using Reply = Message;

  static void FillRequestData(clients::http::Request& request,
                              const Parameters& parameters);

  static Reply ParseResponseData(clients::http::Response& response);
};

SendDiceMethod::Parameters Parse(
    const formats::json::Value& json,
    formats::parse::To<SendDiceMethod::Parameters>);

formats::json::Value Serialize(const SendDiceMethod::Parameters& parameters,
                               formats::serialize::To<formats::json::Value>);

using SendDiceRequest = Request<SendDiceMethod>;

}  // namespace telegram::bot

USERVER_NAMESPACE_END
