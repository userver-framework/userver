#include <userver/telegram/bot/types/reply_keyboard_remove.hpp>

#include <telegram/bot/formats/value_builder.hpp>

#include <userver/formats/json.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

namespace impl {

template <class Value>
ReplyKeyboardRemove Parse(const Value& data,
                          formats::parse::To<ReplyKeyboardRemove>) {
  return ReplyKeyboardRemove{
    data["remove_keyboard"].template As<bool>(),
    data["selective"].template As<std::optional<bool>>()
  };
}

template <class Value>
Value Serialize(const ReplyKeyboardRemove& reply_keyboard_remove,
                formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["remove_keyboard"] = reply_keyboard_remove.remove_keyboard;
  SetIfNotNull(builder, "selective", reply_keyboard_remove.selective);
  return builder.ExtractValue();
}

}  // namespace impl

ReplyKeyboardRemove Parse(const formats::json::Value& json,
                          formats::parse::To<ReplyKeyboardRemove> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const ReplyKeyboardRemove& reply_keyboard_remove,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(reply_keyboard_remove, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
