#pragma once

#include <userver/telegram/bot/types/photo_size.hpp>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief This object represents a video message.
/// @note Available in Telegram apps as of v.4.0/
/// @see https://core.telegram.org/bots/api#videonote
struct VideoNote {
  /// @brief Identifier for this file, which can be used to download
  /// or reuse the file.
  std::string file_id;

  /// @brief Unique identifier for this file, which is supposed to be
  /// the same over time and for different bots.
  /// @note Can't be used to download or reuse the file.
  std::string file_unique_id;

  /// @brief Video width and height (diameter of the video message)
  /// as defined by sender.
  std::int64_t length{};

  /// @brief Duration of the video in seconds as defined by sender.
  std::int64_t duration{};

  /// @brief Optional. Video thumbnail.
  std::unique_ptr<PhotoSize> thumbnail;

  /// @brief Optional. File size in bytes.
  std::optional<std::int64_t> file_size;
};

VideoNote Parse(const formats::json::Value& json,
                formats::parse::To<VideoNote>);

formats::json::Value Serialize(const VideoNote& video_note,
                               formats::serialize::To<formats::json::Value>);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
