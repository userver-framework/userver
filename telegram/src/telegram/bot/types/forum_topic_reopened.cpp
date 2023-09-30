#include <userver/telegram/bot/types/forum_topic_reopened.hpp>

#include <userver/formats/json.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

namespace impl {

template <class Value>
ForumTopicReopened Parse(const Value&, formats::parse::To<ForumTopicReopened>) {
  return ForumTopicReopened{};
}

template <class Value>
Value Serialize(const ForumTopicReopened&, formats::serialize::To<Value>) {
  typename Value::Builder builder;
  return builder.ExtractValue();
}

}  // namespace impl

ForumTopicReopened Parse(const formats::json::Value& json,
                         formats::parse::To<ForumTopicReopened> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const ForumTopicReopened& forum_topic_reopened,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(forum_topic_reopened, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
