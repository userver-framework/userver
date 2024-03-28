#include <userver/telegram/bot/types/dice.hpp>

#include <userver/formats/json.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

namespace impl {

template <class Value>
Dice Parse(const Value& data, formats::parse::To<Dice>) {
  return Dice{
    data["emoji"].template As<std::string>(),
    data["value"].template As<std::int64_t>()
  };
}

template <class Value>
Value Serialize(const Dice& dice, formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["emoji"] = dice.emoji;
  builder["value"] = dice.value;
  return builder.ExtractValue();
}

}  // namespace impl

Dice Parse(const formats::json::Value& json,
           formats::parse::To<Dice> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const Dice& dice,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(dice, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
