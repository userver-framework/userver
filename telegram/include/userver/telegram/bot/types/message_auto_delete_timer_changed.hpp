#pragma once

#include <chrono>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief This object represents a service message about a change
/// in auto-delete timer settings.
/// @see https://core.telegram.org/bots/api#messageautodeletetimerchanged
struct MessageAutoDeleteTimerChanged {
  /// @brief New auto-delete time for messages in the chat.
  std::chrono::seconds message_auto_delete_time{};
};

MessageAutoDeleteTimerChanged Parse(const formats::json::Value& json,
                                    formats::parse::To<MessageAutoDeleteTimerChanged>);

formats::json::Value Serialize(const MessageAutoDeleteTimerChanged& message_auto_delete_timer_changed,
                               formats::serialize::To<formats::json::Value>);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
