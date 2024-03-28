#pragma once

#include <chrono>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief This object represents a service message about
/// a video chat ended in the chat.
/// @see https://core.telegram.org/bots/api#videochatended
struct VideoChatEnded {
  /// @brief Video chat duration in seconds.
  std::chrono::seconds duration{};
};

VideoChatEnded Parse(const formats::json::Value& json,
                     formats::parse::To<VideoChatEnded>);

formats::json::Value Serialize(const VideoChatEnded& video_chat_ended,
                               formats::serialize::To<formats::json::Value>);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
