#include <userver/telegram/bot/types/forum_topic_created.hpp>

#include <telegram/bot/formats/value_builder.hpp>

#include <userver/formats/json.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

namespace impl {

template <class Value>
ForumTopicCreated Parse(const Value& data,
                        formats::parse::To<ForumTopicCreated>) {
  return ForumTopicCreated{
    data["name"].template As<std::string>(),
    data["icon_color"].template As<std::int64_t>(),
    data["icon_custom_emoji_id"].template As<std::optional<std::string>>()
  };
}

template <class Value>
Value Serialize(const ForumTopicCreated& forum_topic_created,
                formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["name"] = forum_topic_created.name;
  builder["icon_color"] = forum_topic_created.icon_color;
  SetIfNotNull(builder, "icon_custom_emoji_id", forum_topic_created.icon_custom_emoji_id);
  return builder.ExtractValue();
}

}  // namespace impl

ForumTopicCreated Parse(const formats::json::Value& json,
                        formats::parse::To<ForumTopicCreated> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const ForumTopicCreated& forum_topic_created,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(forum_topic_created, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
