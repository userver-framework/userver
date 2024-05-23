#include <userver/telegram/bot/types/keyboard_button_request_user.hpp>

#include <telegram/bot/formats/value_builder.hpp>

#include <userver/formats/json.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

namespace impl {

template <class Value>
KeyboardButtonRequestUser Parse(const Value& data,
                                formats::parse::To<KeyboardButtonRequestUser>) {
  return KeyboardButtonRequestUser{
    data["request_id"].template As<std::int64_t>(),
    data["user_is_bot"].template As<std::optional<bool>>(),
    data["user_is_premium"].template As<std::optional<bool>>()
  };
}

template <class Value>
Value Serialize(const KeyboardButtonRequestUser& keyboard_button_request_user,
                formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["request_id"] = keyboard_button_request_user.request_id;
  SetIfNotNull(builder, "user_is_bot", keyboard_button_request_user.user_is_bot);
  SetIfNotNull(builder, "user_is_premium", keyboard_button_request_user.user_is_premium);
  return builder.ExtractValue();
}

}  // namespace impl

KeyboardButtonRequestUser Parse(
    const formats::json::Value& json,
    formats::parse::To<KeyboardButtonRequestUser> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(
    const KeyboardButtonRequestUser& keyboard_button_request_user,
    formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(keyboard_button_request_user, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
