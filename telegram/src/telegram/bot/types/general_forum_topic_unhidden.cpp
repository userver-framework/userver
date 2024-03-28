#include <userver/telegram/bot/types/general_forum_topic_unhidden.hpp>

#include <userver/formats/json.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

namespace impl {

template <class Value>
GeneralForumTopicUnhidden Parse(const Value&,
                                formats::parse::To<GeneralForumTopicUnhidden>) {
  return GeneralForumTopicUnhidden{};
}

template <class Value>
Value Serialize(const GeneralForumTopicUnhidden&,
                formats::serialize::To<Value>) {
  typename Value::Builder builder;
  return builder.ExtractValue();
}

}  // namespace impl

GeneralForumTopicUnhidden Parse(const formats::json::Value& json,
                                formats::parse::To<GeneralForumTopicUnhidden> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const GeneralForumTopicUnhidden& general_forum_topic_unhidden,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(general_forum_topic_unhidden, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
