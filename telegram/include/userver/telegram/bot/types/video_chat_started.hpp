#pragma once

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief This object represents a service message about
/// a video chat started in the chat.
/// @note Currently holds no information.
/// @see https://core.telegram.org/bots/api#videochatstarted
struct VideoChatStarted {};

VideoChatStarted Parse(const formats::json::Value& json,
                       formats::parse::To<VideoChatStarted>);

formats::json::Value Serialize(const VideoChatStarted& video_chat_started,
                               formats::serialize::To<formats::json::Value>);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
