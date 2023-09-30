#pragma once

#include <optional>
#include <string>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief This object represents a parameter of the inline keyboard button 
/// used to automatically authorize a user.
/// @details Serves as a great replacement for the Telegram Login Widget when 
/// the user is coming from Telegram. All the user needs to do is tap/click a
/// button and confirm that they want to log in.
/// @note Telegram apps support these buttons as of version 5.7.
/// @example https://t.me/discussbot
/// @see https://core.telegram.org/bots/api#loginurl
struct LoginUrl {
  /// @brief An HTTPS URL to be opened with user authorization data added to the
  /// query string when the button is pressed. 
  /// @note If the user refuses to provide authorization data, the original URL
  /// without information about the user will be opened.
  /// @see The data added is the same as described in
  /// https://core.telegram.org/widgets/login#receiving-authorization-data
  /// @note You must always check the hash of the received data to verify the
  /// authentication and the integrity of the data as described in
  /// https://core.telegram.org/widgets/login#checking-authorization
  std::string url;

  /// @brief Optional. New text of the button in forwarded messages.
  std::optional<std::string> forward_text;

  /// @brief Optional. Username of a bot, which will be used for user
  /// authorization.
  /// @note If not specified, the current bot's username will be assumed.
  /// @see https://core.telegram.org/widgets/login#setting-up-a-bot
  /// for more details.
  /// @note The url's domain must be the same as the domain linked with the bot.
  /// @see https://core.telegram.org/widgets/login#linking-your-domain-to-the-bot
  /// for more details.
  std::optional<std::string> bot_username;

  /// @brief Optional. Pass True to request the permission for your bot to send
  /// messages to the user.
  std::optional<bool> request_write_access;
};

LoginUrl Parse(const formats::json::Value& json,
               formats::parse::To<LoginUrl>);

formats::json::Value Serialize(const LoginUrl& login_url,
                               formats::serialize::To<formats::json::Value>);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
