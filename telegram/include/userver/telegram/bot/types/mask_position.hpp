#pragma once

#include <string>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief This object describes the position on faces
/// where a mask should be placed by default.
/// @see https://core.telegram.org/bots/api#maskposition
struct MaskPosition {
  enum class Point {
    kForehead, kEyes, kMouth, kChin
  };

  /// @brief The part of the face relative to which the mask should be placed.
  Point point;

  /// @brief Shift by X-axis measured in widths of the mask scaled to
  /// the face size, from left to right. For example, choosing -1.0
  /// will place mask just to the left of the default mask position.
  double x_shift{};

  /// @brief Shift by Y-axis measured in heights of the mask scaled to
  /// the face size, from top to bottom. For example, 1.0 will place the
  /// mask just below the default mask position.
  double y_shift{};

  /// @brief Mask scaling coefficient. For example, 2.0 means double size.
  double scale{};
};

std::string_view ToString(MaskPosition::Point point);

MaskPosition::Point Parse(const formats::json::Value& value,
                          formats::parse::To<MaskPosition::Point>);

formats::json::Value Serialize(MaskPosition::Point point,
                               formats::serialize::To<formats::json::Value>);

MaskPosition Parse(const formats::json::Value& json,
                   formats::parse::To<MaskPosition>);

formats::json::Value Serialize(const MaskPosition& mask_position,
                               formats::serialize::To<formats::json::Value>);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
