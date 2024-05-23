#include <userver/telegram/bot/types/chat_photo.hpp>

#include <userver/formats/json.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

namespace impl {

template <class Value>
ChatPhoto Parse(const Value& data, formats::parse::To<ChatPhoto>) {
  return ChatPhoto{
    data["small_file_id"].template As<std::string>(),
    data["small_file_unique_id"].template As<std::string>(),
    data["big_file_id"].template As<std::string>(),
    data["big_file_unique_id"].template As<std::string>()
  };
}

template <class Value>
Value Serialize(const ChatPhoto& chat_photo, formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["small_file_id"] = chat_photo.small_file_id;
  builder["small_file_unique_id"] = chat_photo.small_file_unique_id;
  builder["big_file_id"] = chat_photo.big_file_id;
  builder["big_file_unique_id"] = chat_photo.big_file_unique_id;
  return builder.ExtractValue();
}

}  // namespace impl

ChatPhoto Parse(const formats::json::Value& json,
                formats::parse::To<ChatPhoto> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const ChatPhoto& chat_photo,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(chat_photo, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
