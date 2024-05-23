#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief This object represents a file ready to be downloaded.
/// @note The maximum file size to download is 20 MB
/// @see https://core.telegram.org/bots/api#file
/// @todo Download method https://api.telegram.org/file/bot<token>/<file_path>
struct File {
  /// @brief Identifier for this file, which can be used to download
  /// or reuse the file.
  std::string file_id;

  /// @brief Unique identifier for this file, which is supposed to be
  /// the same over time and for different bots.
  /// @note Can't be used to download or reuse the file.
  std::string file_unique_id;

  /// @brief Optional. File size in bytes.
  std::optional<std::int64_t> file_size;

  /// @brief Optional. File path.
  std::optional<std::string> file_path;
};

File Parse(const formats::json::Value& json,
           formats::parse::To<File>);

formats::json::Value Serialize(const File& file,
                               formats::serialize::To<formats::json::Value>);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
