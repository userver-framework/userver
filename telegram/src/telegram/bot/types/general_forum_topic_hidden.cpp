#include <userver/telegram/bot/types/general_forum_topic_hidden.hpp>

#include <userver/formats/json.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

namespace impl {

template <class Value>
GeneralForumTopicHidden Parse(const Value&,
                              formats::parse::To<GeneralForumTopicHidden>) {
  return GeneralForumTopicHidden{};
}

template <class Value>
Value Serialize(const GeneralForumTopicHidden&, formats::serialize::To<Value>) {
  typename Value::Builder builder;
  return builder.ExtractValue();
}

}  // namespace impl

GeneralForumTopicHidden Parse(const formats::json::Value& json,
                              formats::parse::To<GeneralForumTopicHidden> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const GeneralForumTopicHidden& general_forum_topic_hidden,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(general_forum_topic_hidden, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
