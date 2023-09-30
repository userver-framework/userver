#pragma once

#include <userver/telegram/bot/types/photo_size.hpp>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief This object represents an audio file to be treated as music
/// by the Telegram clients.
/// @see https://core.telegram.org/bots/api#audio
struct Audio {
  /// @brief Identifier for this file, which can be used to download
  /// or reuse the file.
  std::string file_id;

  /// @brief Unique identifier for this file, which is supposed to be
  /// the same over time and for different bots.
  /// @note Can't be used to download or reuse the file.
  std::string file_unique_id;

  /// @brief Duration of the audio in seconds as defined by sender.
  std::int64_t duration{};

  /// @brief Optional. Performer of the audio as defined by sender or 
  /// by audio tags.
  std::optional<std::string> performer;

  /// @brief Optional. Title of the audio as defined by sender or by audio tags.
  std::optional<std::string> title;

  /// @brief Optional. Original filename as defined by sender.
  std::optional<std::string> file_name;

  /// @brief Optional. MIME type of the file as defined by sender.
  std::optional<std::string> mime_type;

  /// @brief Optional. File size in bytes.
  std::optional<std::int64_t> file_size;

  /// @brief Optional. Thumbnail of the album cover to which
  /// the music file belongs.
  std::unique_ptr<PhotoSize> thumbnail;
};

Audio Parse(const formats::json::Value& json,
            formats::parse::To<Audio>);

formats::json::Value Serialize(const Audio& audio,
                               formats::serialize::To<formats::json::Value>);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
