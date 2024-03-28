#include <userver/telegram/bot/types/video_chat_ended.hpp>

#include <userver/formats/json.hpp>
#include <userver/formats/json/serialize_duration.hpp>
#include <userver/formats/parse/common.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

namespace impl {

template <class Value>
VideoChatEnded Parse(const Value& data, formats::parse::To<VideoChatEnded>) {
  return VideoChatEnded{
    data["duration"].template As<std::chrono::seconds>()
  };
}

template <class Value>
Value Serialize(const VideoChatEnded& video_chat_ended,
                formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["duration"] = video_chat_ended.duration;
  return builder.ExtractValue();
}

}  // namespace impl

VideoChatEnded Parse(const formats::json::Value& json,
                     formats::parse::To<VideoChatEnded> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const VideoChatEnded& video_chat_ended,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(video_chat_ended, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
