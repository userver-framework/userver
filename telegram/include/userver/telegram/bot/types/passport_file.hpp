#pragma once

#include <cstdint>
#include <string>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief This object represents a file uploaded to Telegram Passport.
/// @note Currently all Telegram Passport files are in JPEG format
/// when decrypted and don't exceed 10MB.
/// @see https://core.telegram.org/bots/api#passportfile
struct PassportFile {
  /// @brief Identifier for this file, which can be used to download or
  /// reuse the file
  std::string file_id;

  /// @brief Unique identifier for this file, which is supposed to be
  /// the same over time and for different bots.
  /// @note Can't be used to download or reuse the file.
  std::string file_unique_id;

  /// @brief File size in bytes
  std::int64_t file_size{};

  /// @brief Unix time when the file was uploaded
  std::int64_t file_date{};
};

PassportFile Parse(const formats::json::Value& json,
                   formats::parse::To<PassportFile>);

formats::json::Value Serialize(const PassportFile& passport_file,
                               formats::serialize::To<formats::json::Value>);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
