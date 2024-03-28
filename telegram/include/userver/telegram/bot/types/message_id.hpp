#pragma once

#include <cstdint>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief This object represents a unique message identifier.
/// @see https://core.telegram.org/bots/api#messageid
struct MessageId {
  /// @brief Unique message identifier.
  std::int64_t id{};
};

MessageId Parse(const formats::json::Value& json,
                formats::parse::To<MessageId>);

formats::json::Value Serialize(const MessageId& message_id,
                               formats::serialize::To<formats::json::Value>);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
