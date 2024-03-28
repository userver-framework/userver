#pragma once

#include <string>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief This object represents a chat photo.
/// @see https://core.telegram.org/bots/api#chatphoto
struct ChatPhoto {
  /// @brief File identifier of small (160x160) chat photo.
  /// This file_id can be used only for photo download
  /// and only for as long as the photo is not changed.
  std::string small_file_id;

  /// @brief Unique file identifier of small (160x160) chat photo,
  /// which is supposed to be the same over time and for different bots.
  /// Can't be used to download or reuse the file.
  std::string small_file_unique_id;

  /// @brief File identifier of big (640x640) chat photo.
  /// This file_id can be used only for photo download
  /// and only for as long as the photo is not changed.
  std::string big_file_id;

  /// @brief Unique file identifier of big (640x640) chat photo,
  /// which is supposed to be the same over time and for different bots.
  /// Can't be used to download or reuse the file.
  std::string big_file_unique_id;
};

ChatPhoto Parse(const formats::json::Value& json,
                formats::parse::To<ChatPhoto>);

formats::json::Value Serialize(const ChatPhoto& chat_photo,
                               formats::serialize::To<formats::json::Value>);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
