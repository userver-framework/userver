#pragma once

#include <chrono>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief This object represents a service message about
/// a video chat scheduled in the chat.
/// @see https://core.telegram.org/bots/api#videochatscheduled
struct VideoChatScheduled {
  /// @brief Point in time (Unix timestamp) when the video chat is
  /// supposed to be started by a chat administrator.
  std::chrono::system_clock::time_point start_date{};
};

VideoChatScheduled Parse(const formats::json::Value& json,
                         formats::parse::To<VideoChatScheduled>);

formats::json::Value Serialize(const VideoChatScheduled& video_chat_scheduled,
                               formats::serialize::To<formats::json::Value>);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
