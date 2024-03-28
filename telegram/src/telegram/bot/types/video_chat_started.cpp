#include <userver/telegram/bot/types/video_chat_started.hpp>

#include <userver/formats/json.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

namespace impl {

template <class Value>
VideoChatStarted Parse(const Value&, formats::parse::To<VideoChatStarted>) {
  return VideoChatStarted{};
}

template <class Value>
Value Serialize(const VideoChatStarted&, formats::serialize::To<Value>) {
  typename Value::Builder builder;
  return builder.ExtractValue();
}

}  // namespace impl

VideoChatStarted Parse(const formats::json::Value& json,
                       formats::parse::To<VideoChatStarted> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const VideoChatStarted& video_chat_started,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(video_chat_started, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
