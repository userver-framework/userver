#include <userver/telegram/bot/types/video_chat_scheduled.hpp>

#include <telegram/bot/types/time.hpp>

#include <userver/formats/json.hpp>
#include <userver/formats/json/serialize_duration.hpp>
#include <userver/formats/parse/common.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

namespace impl {

template <class Value>
VideoChatScheduled Parse(const Value& data,
                         formats::parse::To<VideoChatScheduled>) {
  return VideoChatScheduled{
    TransformToTimePoint(data["start_date"].template As<std::chrono::seconds>())
  };
}

template <class Value>
Value Serialize(const VideoChatScheduled& video_chat_scheduled, formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["start_date"] = TransformToSeconds(video_chat_scheduled.start_date);
  return builder.ExtractValue();
}

}  // namespace impl

VideoChatScheduled Parse(const formats::json::Value& json,
                         formats::parse::To<VideoChatScheduled> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const VideoChatScheduled& video_chat_scheduled,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(video_chat_scheduled, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
