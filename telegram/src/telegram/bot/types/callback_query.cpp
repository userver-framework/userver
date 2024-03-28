#include <userver/telegram/bot/types/callback_query.hpp>

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
CallbackQuery Parse(const Value& data, formats::parse::To<CallbackQuery>) {
  return CallbackQuery{
    data["id"].template As<std::string>(),
    data["from"].template As<std::unique_ptr<User>>(),
    data["message"].template As<std::unique_ptr<Message>>(),
    data["inline_message_id"].template As<std::optional<std::string>>(),
    data["chat_instance"].template As<std::string>(),
    data["data"].template As<std::optional<std::string>>(),
    data["game_short_name"].template As<std::optional<std::string>>()
  };
}

template <class Value>
Value Serialize(const CallbackQuery& callback_query,
                formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["id"] = callback_query.id;
  builder["from"] = callback_query.from;
  SetIfNotNull(builder, "message", callback_query.message);
  SetIfNotNull(builder, "inline_message_id", callback_query.inline_message_id);
  builder["chat_instance"] = callback_query.chat_instance;
  SetIfNotNull(builder, "data", callback_query.data);
  SetIfNotNull(builder, "game_short_name", callback_query.game_short_name);
  return builder.ExtractValue();
}

}  // namespace impl

CallbackQuery Parse(const formats::json::Value& json,
                    formats::parse::To<CallbackQuery> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const CallbackQuery& callback_query,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(callback_query, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
