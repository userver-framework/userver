#include <userver/telegram/bot/types/forum_topic_edited.hpp>

#include <telegram/bot/formats/value_builder.hpp>

#include <userver/formats/json.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

namespace impl {

template <class Value>
ForumTopicEdited Parse(const Value& data,
                       formats::parse::To<ForumTopicEdited>) {
  return ForumTopicEdited{
    data["name"].template As<std::optional<std::string>>(),
    data["icon_custom_emoji_id"].template As<std::optional<std::string>>()
  };
}

template <class Value>
Value Serialize(const ForumTopicEdited& forum_topic_edited,
                formats::serialize::To<Value>) {
  typename Value::Builder builder;
  SetIfNotNull(builder, "name", forum_topic_edited.name);
  SetIfNotNull(builder, "icon_custom_emoji_id", forum_topic_edited.icon_custom_emoji_id);
  return builder.ExtractValue();
}

}  // namespace impl

ForumTopicEdited Parse(const formats::json::Value& json,
                       formats::parse::To<ForumTopicEdited> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const ForumTopicEdited& forum_topic_edited,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(forum_topic_edited, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
