#include <userver/telegram/bot/types/chat_shared.hpp>

#include <userver/formats/json.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

namespace impl {

template <class Value>
ChatShared Parse(const Value& data, formats::parse::To<ChatShared>) {
  return ChatShared{
    data["request_id"].template As<std::int64_t>(),
    data["chat_id"].template As<std::int64_t>()
  };
}

template <class Value>
Value Serialize(const ChatShared& chat_shared, formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["request_id"] = chat_shared.request_id;
  builder["chat_id"] = chat_shared.chat_id;
  return builder.ExtractValue();
}

}  // namespace impl

ChatShared Parse(const formats::json::Value& json,
                 formats::parse::To<ChatShared> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const ChatShared& chat_shared,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(chat_shared, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
