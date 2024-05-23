#pragma once

#include <optional>
#include <string>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief This object represents a service message about an edited forum topic.
/// @see https://core.telegram.org/bots/api#forumtopicedited
struct ForumTopicEdited {
  /// @brief Optional. New name of the topic, if it was edited.
  std::optional<std::string> name;

  /// @brief Optional. New identifier of the custom emoji shown as
  /// the topic icon, if it was edited.
  /// @note An empty string if the icon was removed
  std::optional<std::string> icon_custom_emoji_id;
};

ForumTopicEdited Parse(const formats::json::Value& json,
                       formats::parse::To<ForumTopicEdited>);

formats::json::Value Serialize(const ForumTopicEdited& forum_topic_edited,
                               formats::serialize::To<formats::json::Value>);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
