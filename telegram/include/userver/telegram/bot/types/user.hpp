#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief This object represents a Telegram user or bot.
/// @see https://core.telegram.org/bots/api#user
struct User {
  /// @brief Unique identifier for this user or bot.
  std::int64_t id{};

  /// @brief True, if this user is a bot.
  bool is_bot{};

  /// @brief User's or bot's first name.
  std::string first_name;

  /// @brief Optional. User's or bot's last name.
  std::optional<std::string> last_name;

  /// @brief Optional. User's or bot's username.
  std::optional<std::string> username;

  /// @brief Optional. IETF language tag of the user's language.
  /// @see https://en.wikipedia.org/wiki/IETF_language_tag
  std::optional<std::string> language_code;

  /// @brief Optional. True, if this user is a Telegram Premium user.
  std::optional<bool> is_premium;

  /// @brief Optional. True, if this user added the bot to the attachment menu.
  std::optional<bool> added_to_attachment_menu;

  /// @brief Optional. True, if the bot can be invited to groups.
  /// @note Returned only in GetMe.
  std::optional<bool> can_join_groups;

  /// @brief Optional. True, if privacy mode is disabled for the bot.
  /// @note Returned only in GetMe.
  /// @see https://core.telegram.org/bots/features#privacy-mode
  std::optional<bool> can_read_all_group_messages;

  /// @brief Optional. True, if the bot supports inline queries.
  /// @note Returned only in GetMe.
  std::optional<bool> supports_inline_queries;
};

User Parse(const formats::json::Value& json,
           formats::parse::To<User>);

formats::json::Value Serialize(const User& user,
                               formats::serialize::To<formats::json::Value>);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
