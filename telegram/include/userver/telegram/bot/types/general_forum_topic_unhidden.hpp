#pragma once

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief This object represents a service message about General
/// forum topic unhidden in the chat.
/// @note Currently holds no information.
/// @see https://core.telegram.org/bots/api#generalforumtopicunhidden
struct GeneralForumTopicUnhidden {};

GeneralForumTopicUnhidden Parse(const formats::json::Value& json,
                                formats::parse::To<GeneralForumTopicUnhidden>);

formats::json::Value Serialize(const GeneralForumTopicUnhidden& general_forum_topic_unhidden,
                               formats::serialize::To<formats::json::Value>);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
