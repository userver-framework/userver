#pragma once

#include <userver/telegram/bot/types/location.hpp>

#include <memory>
#include <string>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief Represents a location to which a chat is connected.
/// @see https://core.telegram.org/bots/api#chatlocation
struct ChatLocation {
  /// @brief The location to which the supergroup is connected.
  /// @note Can't be a live location.
  std::unique_ptr<Location> location;

  /// @brief Location address; 1-64 characters, as defined by the chat owner.
  std::string address;
};

ChatLocation Parse(const formats::json::Value& json,
                   formats::parse::To<ChatLocation>);

formats::json::Value Serialize(const ChatLocation& chat_location,
                               formats::serialize::To<formats::json::Value>);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
