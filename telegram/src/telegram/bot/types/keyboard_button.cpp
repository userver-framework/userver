#include <userver/telegram/bot/types/keyboard_button.hpp>

#include <telegram/bot/formats/parse.hpp>
#include <telegram/bot/formats/serialize.hpp>
#include <telegram/bot/formats/value_builder.hpp>

#include <userver/formats/json.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

namespace impl {

template <class Value>
KeyboardButton Parse(const Value& data, formats::parse::To<KeyboardButton>) {
  return KeyboardButton{
    data["text"].template As<std::string>(),
    data["request_user"].template As<std::unique_ptr<KeyboardButtonRequestUser>>(),
    data["request_chat"].template As<std::unique_ptr<KeyboardButtonRequestChat>>(),
    data["request_contact"].template As<std::optional<bool>>(),
    data["request_location"].template As<std::optional<bool>>(),
    data["request_poll"].template As<std::unique_ptr<KeyboardButtonPollType>>(),
    data["web_app"].template As<std::unique_ptr<WebAppInfo>>()
  };
}

template <class Value>
Value Serialize(const KeyboardButton& keyboard_button,
                formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["text"] = keyboard_button.text;
  SetIfNotNull(builder, "request_user", keyboard_button.request_user);
  SetIfNotNull(builder, "request_chat", keyboard_button.request_chat);
  SetIfNotNull(builder, "request_contact", keyboard_button.request_contact);
  SetIfNotNull(builder, "request_location", keyboard_button.request_location);
  SetIfNotNull(builder, "request_poll", keyboard_button.request_poll);
  SetIfNotNull(builder, "web_app", keyboard_button.web_app);
  return builder.ExtractValue();
}

}  // namespace impl

KeyboardButton Parse(const formats::json::Value& json,
                     formats::parse::To<KeyboardButton> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(
    const KeyboardButton& keyboard_button,
    formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(keyboard_button, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
