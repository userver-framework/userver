#pragma once

#include <cstdint>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief This object contains information about the user whose
/// identifier was shared with the bot using a KeyboardButtonRequestUser button.
/// @see https://core.telegram.org/bots/api#usershared
struct UserShared {
  /// @brief Identifier of the request
  std::int64_t request_id{};

  /// @brief Identifier of the shared user.
  /// @note The bot may not have access to the user and could be unable to use
  /// this identifier, unless the user is already known to the bot by some
  /// other means.
  std::int64_t user_id{};
};

UserShared Parse(const formats::json::Value& json,
                 formats::parse::To<UserShared>);

formats::json::Value Serialize(const UserShared& user_shared,
                               formats::serialize::To<formats::json::Value>);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
