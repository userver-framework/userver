#pragma once

#include <userver/telegram/bot/types/photo_size.hpp>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief This object represents a video file.
/// @see https://core.telegram.org/bots/api#video
struct Video {
  /// @brief Identifier for this file, which can be used to download
  /// or reuse the file.
  std::string file_id;

  /// @brief Unique identifier for this file, which is supposed to be
  /// the same over time and for different bots.
  /// @note Can't be used to download or reuse the file.
  std::string file_unique_id;

  /// @brief Video width as defined by sender.
  std::int64_t width{};

  /// @brief Video height as defined by sender.
  std::int64_t height{};

  /// @brief Duration of the video in seconds as defined by sender.
  std::int64_t duration{};

  /// @brief Optional. Video thumbnail.
  std::unique_ptr<PhotoSize> thumbnail;

  /// @brief Optional. Original filename as defined by sender.
  std::optional<std::string> file_name;

  /// @brief Optional. MIME type of the file as defined by sender.
  std::optional<std::string> mime_type;

  /// @brief Optional. File size in bytes.
  std::optional<std::int64_t> file_size;
};

Video Parse(const formats::json::Value& json,
            formats::parse::To<Video>);

formats::json::Value Serialize(const Video& video,
                               formats::serialize::To<formats::json::Value>);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
