#include <userver/telegram/bot/types/message_id.hpp>

#include <userver/formats/json.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

namespace impl {

template <class Value>
MessageId Parse(const Value& data, formats::parse::To<MessageId>) {
  return MessageId{
    data["message_id"].template As<std::int64_t>()
  };
}

template <class Value>
Value Serialize(const MessageId& message_id, formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["message_id"] = message_id.id;
  return builder.ExtractValue();
}

}  // namespace impl

MessageId Parse(const formats::json::Value& json,
                formats::parse::To<MessageId> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const MessageId& message_id,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(message_id, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
