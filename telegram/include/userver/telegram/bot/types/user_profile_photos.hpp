#pragma once

#include <userver/telegram/bot/types/photo_size.hpp>

#include <cstdint>
#include <vector>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief This object represent a user's profile pictures.
/// @see https://core.telegram.org/bots/api#userprofilephotos
struct UserProfilePhotos {
  /// @brief Total number of profile pictures the target user has.
  std::int64_t total_count{};

  /// @brief Requested profile pictures (in up to 4 sizes each)
  std::vector<std::vector<PhotoSize>> photos;
};

UserProfilePhotos Parse(const formats::json::Value& json,
                        formats::parse::To<UserProfilePhotos>);

formats::json::Value Serialize(const UserProfilePhotos& user_profile_photos,
                               formats::serialize::To<formats::json::Value>);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
