#pragma once

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief This object represents a message about a forwarded story in the chat.
/// @note Currently holds no information.
/// @see https://core.telegram.org/bots/api#story
struct Story {};

Story Parse(const formats::json::Value& json,
            formats::parse::To<Story>);

formats::json::Value Serialize(const Story& story,
                               formats::serialize::To<formats::json::Value>);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
