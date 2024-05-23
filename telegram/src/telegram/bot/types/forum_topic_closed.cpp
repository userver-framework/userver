#include <userver/telegram/bot/types/forum_topic_closed.hpp>

#include <userver/formats/json.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

namespace impl {

template <class Value>
ForumTopicClosed Parse(const Value&, formats::parse::To<ForumTopicClosed>) {
  return ForumTopicClosed{};
}

template <class Value>
Value Serialize(const ForumTopicClosed&, formats::serialize::To<Value>) {
  typename Value::Builder builder;
  return builder.ExtractValue();
}

}  // namespace impl

ForumTopicClosed Parse(const formats::json::Value& json,
                       formats::parse::To<ForumTopicClosed> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const ForumTopicClosed& forum_topic_closed,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(forum_topic_closed, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
