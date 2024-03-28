#include <userver/telegram/bot/types/story.hpp>

#include <userver/formats/json.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

namespace impl {

template <class Value>
Story Parse(const Value&, formats::parse::To<Story>) {
  return Story{};
}

template <class Value>
Value Serialize(const Story&, formats::serialize::To<Value>) {
  typename Value::Builder builder;
  return builder.ExtractValue();
}

}  // namespace impl

Story Parse(const formats::json::Value& json,
            formats::parse::To<Story> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const Story& story,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(story, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
