#pragma once

#include <userver/telegram/bot/types/location.hpp>

#include <memory>
#include <optional>
#include <string>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief This object represents a venue.
/// @see https://core.telegram.org/bots/api#venue
struct Venue {
  /// @brief Venue location. Can't be a live location
  std::unique_ptr<Location> location;

  /// @brief Name of the venue
  std::string title;

  /// @brief Address of the venue
  std::string address;

  /// @brief Optional. Foursquare identifier of the venue
  std::optional<std::string> foursquare_id;

  /// @brief Optional. Foursquare type of the venue.
  /// (For example, “arts_entertainment/default”, “arts_entertainment/aquarium”
  /// or “food/icecream”.)
  std::optional<std::string> foursquare_type;

  /// @brief Optional. Google Places identifier of the venue
  std::optional<std::string> google_place_id;

  /// @brief Optional. Google Places type of the venue.
  /// @see https://developers.google.com/maps/documentation/places/web-service/supported_types
  std::optional<std::string> google_place_type;
};

Venue Parse(const formats::json::Value& json,
            formats::parse::To<Venue>);

formats::json::Value Serialize(const Venue& venue,
                               formats::serialize::To<formats::json::Value>);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
