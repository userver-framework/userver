#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief This object represents a voice note.
/// @see https://core.telegram.org/bots/api#voice
struct Voice {
  /// @brief Identifier for this file, which can be used to download
  /// or reuse the file.
  std::string file_id;

  /// @brief Unique identifier for this file, which is supposed to be
  /// the same over time and for different bots.
  /// @note Can't be used to download or reuse the file.
  std::string file_unique_id;

  /// @brief Duration of the audio in seconds as defined by sender.
  std::int64_t duration{};

  /// @brief Optional. MIME type of the file as defined by sender.
  std::optional<std::string> mime_type;

  /// @brief Optional. File size in bytes.
  std::optional<std::int64_t> file_size;
};

Voice Parse(const formats::json::Value& json,
            formats::parse::To<Voice>);

formats::json::Value Serialize(const Voice& voice,
                               formats::serialize::To<formats::json::Value>);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
