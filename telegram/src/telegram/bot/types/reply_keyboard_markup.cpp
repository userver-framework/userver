#include <userver/telegram/bot/types/reply_keyboard_markup.hpp>

#include <telegram/bot/formats/value_builder.hpp>

#include <userver/formats/json.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

namespace impl {

template <class Value>
ReplyKeyboardMarkup Parse(const Value& data, 
                          formats::parse::To<ReplyKeyboardMarkup>) {
  return ReplyKeyboardMarkup{
    data["keyboard"].template As<std::vector<std::vector<KeyboardButton>>>(),
    data["is_persistent"].template As<std::optional<bool>>(),
    data["resize_keyboard"].template As<std::optional<bool>>(),
    data["one_time_keyboard"].template As<std::optional<bool>>(),
    data["input_field_placeholder"].template As<std::optional<std::string>>(),
    data["selective"].template As<std::optional<bool>>()
  };
}

template <class Value>
Value Serialize(const ReplyKeyboardMarkup& reply_keyboard_markup,
                formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["keyboard"] = reply_keyboard_markup.keyboard;
  SetIfNotNull(builder, "is_persistent", reply_keyboard_markup.is_persistent);
  SetIfNotNull(builder, "resize_keyboard", reply_keyboard_markup.resize_keyboard);
  SetIfNotNull(builder, "one_time_keyboard", reply_keyboard_markup.one_time_keyboard);
  SetIfNotNull(builder, "input_field_placeholder", reply_keyboard_markup.input_field_placeholder);
  SetIfNotNull(builder, "selective", reply_keyboard_markup.selective);
  return builder.ExtractValue();
}

}  // namespace impl

ReplyKeyboardMarkup Parse(const formats::json::Value& json,
                          formats::parse::To<ReplyKeyboardMarkup> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const ReplyKeyboardMarkup& reply_keyboard_markup,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(reply_keyboard_markup, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
