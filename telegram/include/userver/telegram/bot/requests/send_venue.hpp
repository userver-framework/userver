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

/// @brief Use this method to send information about a venue.
/// @see https://core.telegram.org/bots/api#sendvenue
struct SendVenueMethod {
  static constexpr std::string_view kName = "sendVenue";

  static constexpr auto kHttpMethod = clients::http::HttpMethod::kPost;

  struct Parameters{
    Parameters(ChatId _chat_id,
               double _latitude,
               double _longitude,
               std::string _title,
               std::string _address);

    /// @brief Unique identifier for the target chat or username of the target
    /// channel (in the format @channelusername).
    ChatId chat_id;

    /// @brief Unique identifier for the target message thread (topic) of
    /// the forum.
    /// @note For forum supergroups only.
    std::optional<std::int64_t> message_thread_id;

    /// @brief Latitude of the venue.
    double latitude{};

    /// @brief Longitude of the venue.
    double longitude{};

    /// @brief Name of the venue.
    std::string title;

    /// @brief Address of the venue.
    std::string address;

    /// @brief Foursquare identifier of the venue.
    std::optional<std::string> foursquare_id;

    /// @brief Foursquare type of the venue, if known.
    /// @example "arts_entertainment/default", "arts_entertainment/aquarium",
    /// "food/icecream".
    std::optional<std::string> foursquare_type;

    /// @brief Google Places identifier of the venue.
    std::optional<std::string> google_place_id;

    /// @brief Google Places type of the venue.
    /// @see https://developers.google.com/maps/documentation/places/web-service/supported_types
    std::optional<std::string> google_place_type;

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

SendVenueMethod::Parameters Parse(const formats::json::Value& json,
                                  formats::parse::To<SendVenueMethod::Parameters>);

formats::json::Value Serialize(const SendVenueMethod::Parameters& parameters,
                               formats::serialize::To<formats::json::Value>);

using SendVenueRequest = Request<SendVenueMethod>;

}  // namespace telegram::bot

USERVER_NAMESPACE_END
