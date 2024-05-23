#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief This object represents one size of a photo or
/// a file / sticker thumbnail.
/// @see https://core.telegram.org/bots/api#photosize
struct PhotoSize {
  /// @brief Identifier for this file, which can be used to download
  /// or reuse the file.
  std::string file_id;

  /// @brief Unique identifier for this file, which is supposed to be the
  /// same over time and for different bots.
  /// @note Can't be used to download or reuse the file.
  std::string file_unique_id;

  /// @brief Photo width.
  std::int64_t width{};

  /// @brief Photo height.
  std::int64_t height{};

  /// @brief Optional. File size in bytes.
  std::optional<std::int64_t> file_size;
};

PhotoSize Parse(const formats::json::Value& json,
                formats::parse::To<PhotoSize>);

formats::json::Value Serialize(const PhotoSize& photo_size,
                               formats::serialize::To<formats::json::Value>);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
