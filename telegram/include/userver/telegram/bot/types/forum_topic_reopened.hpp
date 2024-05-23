#pragma once

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief This object represents a service message about
/// a forum topic reopened in the chat.
/// @note Currently holds no information.
/// @see https://core.telegram.org/bots/api#forumtopicreopened
struct ForumTopicReopened {};

ForumTopicReopened Parse(const formats::json::Value& json,
                         formats::parse::To<ForumTopicReopened>);

formats::json::Value Serialize(const ForumTopicReopened& forum_topic_reopened,
                               formats::serialize::To<formats::json::Value>);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
