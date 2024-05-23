#include <userver/telegram/bot/types/keyboard_button_request_chat.hpp>

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
KeyboardButtonRequestChat Parse(const Value& data,
                                formats::parse::To<KeyboardButtonRequestChat>) {
  return KeyboardButtonRequestChat{
    data["request_id"].template As<std::int64_t>(),
    data["chat_is_channel"].template As<bool>(),
    data["chat_is_forum"].template As<std::optional<bool>>(),
    data["chat_has_username"].template As<std::optional<bool>>(),
    data["chat_is_created"].template As<std::optional<bool>>(),
    data["user_administrator_rights"].template As<std::unique_ptr<ChatAdministratorRights>>(),
    data["bot_administrator_rights"].template As<std::unique_ptr<ChatAdministratorRights>>(),
    data["bot_is_member"].template As<std::optional<bool>>()
  };
}

template <class Value>
Value Serialize(const KeyboardButtonRequestChat& keyboard_button_request_chat,
                formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["request_id"] = keyboard_button_request_chat.request_id;
  builder["chat_is_channel"] = keyboard_button_request_chat.chat_is_channel;
  SetIfNotNull(builder, "chat_is_forum", keyboard_button_request_chat.chat_is_forum);
  SetIfNotNull(builder, "chat_has_username", keyboard_button_request_chat.chat_has_username);
  SetIfNotNull(builder, "chat_is_created", keyboard_button_request_chat.chat_is_created);
  SetIfNotNull(builder, "user_administrator_rights", keyboard_button_request_chat.user_administrator_rights);
  SetIfNotNull(builder, "bot_administrator_rights", keyboard_button_request_chat.bot_administrator_rights);
  SetIfNotNull(builder, "bot_is_member", keyboard_button_request_chat.bot_is_member);
  return builder.ExtractValue();
}

}  // namespace impl

KeyboardButtonRequestChat Parse(
    const formats::json::Value& json,
    formats::parse::To<KeyboardButtonRequestChat> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(
    const KeyboardButtonRequestChat& keyboard_button_request_chat,
    formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(keyboard_button_request_chat, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
