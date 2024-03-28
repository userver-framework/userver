#pragma once

#include <cstdint>
#include <optional>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief This object represents a point on the map.
/// @see https://core.telegram.org/bots/api#location
struct Location {
  /// @brief Longitude as defined by sender.
  double longitude{};

  /// @brief Latitude as defined by sender.
  double latitude{};

  /// @brief Optional. The radius of uncertainty for the location,
  /// measured in meters; 0-1500.
  std::optional<double> horizontal_accuracy;

  /// @brief Optional. Time relative to the message sending date,
  /// during which the location can be updated; in seconds.
  /// @note For active live locations only.
  std::optional<std::int64_t> live_period;

  /// @brief Optional. The direction in which user is moving, in degrees; 1-360.
  /// @note For active live locations only.
  std::optional<std::int64_t> heading;

  /// @brief Optional. The maximum distance for proximity alerts about
  /// approaching another chat member, in meters.
  /// @note For sent live locations only.
  std::optional<std::int64_t> proximity_alert_radius;
};

Location Parse(const formats::json::Value& json,
               formats::parse::To<Location>);

formats::json::Value Serialize(const Location& location,
                               formats::serialize::To<formats::json::Value>);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
