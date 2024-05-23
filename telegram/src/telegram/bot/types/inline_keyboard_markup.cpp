#include <userver/telegram/bot/types/inline_keyboard_markup.hpp>

#include <userver/formats/json.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

namespace impl {

template <class Value>
InlineKeyboardMarkup Parse(const Value& data,
                           formats::parse::To<InlineKeyboardMarkup>) {
  return InlineKeyboardMarkup{
    data["inline_keyboard"].template As<std::vector<std::vector<InlineKeyboardButton>>>()
  };
}

template <class Value>
Value Serialize(const InlineKeyboardMarkup& inline_keyboard_markup,
                formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["inline_keyboard"] = inline_keyboard_markup.inline_keyboard;
  return builder.ExtractValue();
}

}  // namespace impl

InlineKeyboardMarkup Parse(const formats::json::Value& json,
                           formats::parse::To<InlineKeyboardMarkup> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const InlineKeyboardMarkup& inline_keyboard_markup,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(inline_keyboard_markup, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
