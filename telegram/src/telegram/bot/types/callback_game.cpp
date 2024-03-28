#include <userver/telegram/bot/types/callback_game.hpp>

#include <userver/formats/json.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

namespace impl {

template <class Value>
CallbackGame Parse(const Value&, formats::parse::To<CallbackGame>) {
  return CallbackGame{};
}

template <class Value>
Value Serialize(const CallbackGame&, formats::serialize::To<Value>) {
  typename Value::Builder builder;
  return builder.ExtractValue();
}

}  // namespace impl

CallbackGame Parse(const formats::json::Value& json,
                   formats::parse::To<CallbackGame> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const CallbackGame& callback_game,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(callback_game, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
