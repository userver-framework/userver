#pragma once

#include <userver/telegram/bot/types/location.hpp>
#include <userver/telegram/bot/types/user.hpp>

#include <memory>
#include <optional>
#include <string>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief This object represents an incoming inline query.
/// When the user sends an empty query, your bot could return some default or
/// trending results.
/// @see https://core.telegram.org/bots/api#inlinequery
struct InlineQuery {
  /// @brief Unique identifier for this query.
  std::string id;

  /// @brief Sender.
  std::unique_ptr<User> from;

  /// @brief Text of the query (up to 256 characters).
  std::string query;

  /// @brief Offset of the results to be returned, can be controlled by the bot.
  std::string offset;

  enum class ChatType {
    kPrivate, kGroup, kSupergroup, kChannel,
    kSender, ///< For a private chat with the inline query sender.
  };

  /// @brief Optional. Type of the chat from which the inline query was sent.
  /// @note The chat type should be always known for requests sent from
  /// official clients and most third-party clients, unless the request was
  /// sent from a secret chat.
  std::optional<ChatType> chat_type;

  /// @brief Optional. Sender location, only for bots that request user
  /// location.
  std::unique_ptr<Location> location;
};

std::string_view ToString(InlineQuery::ChatType chat_type);

InlineQuery::ChatType Parse(const formats::json::Value& value,
                            formats::parse::To<InlineQuery::ChatType>);

formats::json::Value Serialize(InlineQuery::ChatType chat_type,
                               formats::serialize::To<formats::json::Value>);

InlineQuery Parse(const formats::json::Value& json,
                  formats::parse::To<InlineQuery>);

formats::json::Value Serialize(const InlineQuery& inline_query,
                               formats::serialize::To<formats::json::Value>);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
