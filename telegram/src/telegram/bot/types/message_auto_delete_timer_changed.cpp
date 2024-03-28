#include <userver/telegram/bot/types/message_auto_delete_timer_changed.hpp>

#include <userver/formats/json.hpp>
#include <userver/formats/json/serialize_duration.hpp>
#include <userver/formats/parse/common.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

namespace impl {

template <class Value>
MessageAutoDeleteTimerChanged Parse(const Value& data,
                                    formats::parse::To<MessageAutoDeleteTimerChanged>) {
  return MessageAutoDeleteTimerChanged{
    data["message_auto_delete_time"].template As<std::chrono::seconds>()
  };
}

template <class Value>
Value Serialize(const MessageAutoDeleteTimerChanged& message_auto_delete_timer_changed,
                formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["message_auto_delete_time"] = message_auto_delete_timer_changed.message_auto_delete_time;
  return builder.ExtractValue();
}

}  // namespace impl

MessageAutoDeleteTimerChanged Parse(const formats::json::Value& json,
                                    formats::parse::To<MessageAutoDeleteTimerChanged> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const MessageAutoDeleteTimerChanged& message_auto_delete_timer_changed,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(message_auto_delete_timer_changed, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
