#pragma once

#include <userver/telegram/bot/types/file.hpp>
#include <userver/telegram/bot/types/mask_position.hpp>
#include <userver/telegram/bot/types/photo_size.hpp>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief This object represents a sticker.
/// @see https://core.telegram.org/bots/api#sticker
struct Sticker {
  /// @brief Identifier for this file, which can be used to download
  /// or reuse the file.
  std::string file_id;

  /// @brief Unique identifier for this file, which is supposed to be
  /// the same over time and for different bots.
  /// @note Can't be used to download or reuse the file.
  std::string file_unique_id;

  enum class Type {
    kRegular, kMask, kCustomEmoji
  };

  /// @brief Type of the sticker.
  /// @note The type of the sticker is independent from its format,
  /// which is determined by the fields is_animated and is_video.
  Type type;

  /// @brief Sticker width.
  std::int64_t width{};

  /// @brief Sticker height.
  std::int64_t height{};

  /// @brief True, if the sticker is animated.
  bool is_animated{};

  /// @brief True, if the sticker is a video sticker.
  bool is_video{};

  /// @brief Optional. Sticker thumbnail in the .WEBP or .JPG format.
  std::unique_ptr<PhotoSize> thumbnail;

  /// @brief Optional. Emoji associated with the sticker.
  std::optional<std::string> emoji;

  /// @brief Optional. Name of the sticker set to which the sticker belongs.
  std::optional<std::string> set_name;

  /// @brief Optional. For premium regular stickers,
  /// premium animation for the sticker.
  std::unique_ptr<File> premium_animation;

  /// @brief Optional. For mask stickers, the position where
  /// the mask should be placed.
  std::unique_ptr<MaskPosition> mask_position;

  /// @brief Optional. For custom emoji stickers, unique identifier
  /// of the custom emoji.
  std::optional<std::string> custom_emoji_id;

  /// @brief Optional. True, if the sticker must be repainted to a text
  /// color in messages, the color of the Telegram Premium badge in
  /// emoji status, white color on chat photos, or another appropriate
  /// color in other places.
  std::optional<bool> needs_repainting;

  /// @brief Optional. File size in bytes
  std::optional<std::int64_t> file_size;
};

Sticker Parse(const formats::json::Value& json,
              formats::parse::To<Sticker>);

formats::json::Value Serialize(const Sticker& sticker,
                               formats::serialize::To<formats::json::Value>);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
