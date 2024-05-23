#pragma once

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief This object represents a service message about a forum
/// topic closed in the chat.
/// @note Currently holds no information.
/// @see https://core.telegram.org/bots/api#forumtopicclosed
struct ForumTopicClosed {};

ForumTopicClosed Parse(const formats::json::Value& json,
                       formats::parse::To<ForumTopicClosed>);

formats::json::Value Serialize(const ForumTopicClosed& forum_topic_closed,
                               formats::serialize::To<formats::json::Value>);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
