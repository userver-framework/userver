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

/// @brief Use this method to send point on the map.
/// @see https://core.telegram.org/bots/api#sendlocation
struct SendLocationMethod {
  static constexpr std::string_view kName = "sendLocation";

  static constexpr auto kHttpMethod = clients::http::HttpMethod::kPost;

  struct Parameters{
    Parameters(ChatId _chat_id,
               double _latitude,
               double _longitude);

    /// @brief Unique identifier for the target chat or username of the
    /// target channel (in the format @channelusername).
    ChatId chat_id;

    /// @brief Unique identifier for the target message thread (topic)
    /// of the forum.
    /// @note For forum supergroups only.
    std::optional<std::int64_t> message_thread_id;

    /// @brief Latitude of the location.
    double latitude{};

    /// @brief Longitude of the location.
    double longitude{};

    /// @brief The radius of uncertainty for the location, measured in meters;
    /// 0-1500
    std::optional<double> horizontal_accuracy;

    /// @brief Period in seconds for which the location will be updated.
    /// @note Should be between 60 and 86400.
    /// @see https://telegram.org/blog/live-locations
    std::optional<std::int64_t> live_period;

    /// @brief Direction in which the user is moving, in degrees.
    /// @note For live locations
    /// @note Must be between 1 and 360 if specified.
    std::optional<std::int64_t> heading;

    /// @brief A maximum distance for proximity alerts about approaching
    /// another chat member, in meters.
    /// @note For live locations
    /// @note Must be between 1 and 100000 if specified.
    std::optional<std::int64_t> proximity_alert_radius;

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

  /// @brief On success, the sent Message is returned.
  using Reply = Message;

  static void FillRequestData(clients::http::Request& request,
                              const Parameters& parameters);

  static Reply ParseResponseData(clients::http::Response& response);
};

SendLocationMethod::Parameters Parse(
    const formats::json::Value& json,
    formats::parse::To<SendLocationMethod::Parameters>);

formats::json::Value Serialize(const SendLocationMethod::Parameters& parameters,
                               formats::serialize::To<formats::json::Value>);

using SendLocationRequest = Request<SendLocationMethod>;

}  // namespace telegram::bot

USERVER_NAMESPACE_END
