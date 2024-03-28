#pragma once

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief This object represents a service message about General
/// forum topic hidden in the chat.
/// @note Currently holds no information.
/// @see https://core.telegram.org/bots/api#generalforumtopichidden
struct GeneralForumTopicHidden {};

GeneralForumTopicHidden Parse(const formats::json::Value& json,
                              formats::parse::To<GeneralForumTopicHidden>);

formats::json::Value Serialize(const GeneralForumTopicHidden& general_forum_topic_hidden,
                               formats::serialize::To<formats::json::Value>);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
