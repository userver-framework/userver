#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief This object represents a service message about a new
/// forum topic created in the chat.
/// @see https://core.telegram.org/bots/api#forumtopiccreated
struct ForumTopicCreated {
  /// @brief Name of the topic.
  std::string name;

  /// @brief Color of the topic icon in RGB format.
  std::int64_t icon_color{};

  /// @brief Optional. Unique identifier of the custom emoji 
  /// shown as the topic icon.
  std::optional<std::string> icon_custom_emoji_id;
};

ForumTopicCreated Parse(const formats::json::Value& json,
                        formats::parse::To<ForumTopicCreated>);

formats::json::Value Serialize(const ForumTopicCreated& forum_topic_created,
                               formats::serialize::To<formats::json::Value>);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
