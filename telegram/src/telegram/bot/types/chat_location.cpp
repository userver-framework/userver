#include <userver/telegram/bot/types/chat_location.hpp>

#include <telegram/bot/formats/parse.hpp>
#include <telegram/bot/formats/serialize.hpp>
#include <telegram/bot/formats/value_builder.hpp>

#include <userver/formats/json.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

namespace impl {

template <class Value>
ChatLocation Parse(const Value& data, formats::parse::To<ChatLocation>) {
  return ChatLocation{
    data["location"].template As<std::unique_ptr<Location>>(),
    data["address"].template As<std::string>()
  };
}

template <class Value>
Value Serialize(const ChatLocation& chat_location,
                formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["location"] = chat_location.location;
  builder["address"] = chat_location.address;
  return builder.ExtractValue();
}

}  // namespace impl

ChatLocation Parse(const formats::json::Value& json,
                   formats::parse::To<ChatLocation> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const ChatLocation& chat_location,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(chat_location, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
