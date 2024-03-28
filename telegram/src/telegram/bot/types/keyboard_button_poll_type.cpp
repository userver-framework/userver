#include <userver/telegram/bot/types/keyboard_button_poll_type.hpp>

#include <telegram/bot/formats/value_builder.hpp>

#include <userver/formats/json.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/utils/trivial_map.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

namespace {

constexpr utils::TrivialBiMap kPollTypeMap([](auto selector) {
  return selector()
    .Case(KeyboardButtonPollType::PoolType::kQuiz, "quiz")
    .Case(KeyboardButtonPollType::PoolType::kRegular, "regular")
    .Case(KeyboardButtonPollType::PoolType::kAny, "any");
});

}  // namespace

namespace impl {

template <class Value>
KeyboardButtonPollType Parse(const Value& data,
                             formats::parse::To<KeyboardButtonPollType>) {
  return KeyboardButtonPollType{
    data["type"].template As<std::optional<KeyboardButtonPollType::PoolType>>()
  };
}

template <class Value>
Value Serialize(const KeyboardButtonPollType& keyboard_button_poll_type,
                formats::serialize::To<Value>) {
  typename Value::Builder builder;
  SetIfNotNull(builder, "type", keyboard_button_poll_type.type);
  return builder.ExtractValue();
}

}  // namespace impl

std::string_view ToString(KeyboardButtonPollType::PoolType poll_type) {
  return utils::impl::EnumToStringView(poll_type, kPollTypeMap);
}

KeyboardButtonPollType::PoolType Parse(
    const formats::json::Value& value,
    formats::parse::To<KeyboardButtonPollType::PoolType>) {
  return utils::ParseFromValueString(value, kPollTypeMap);
}

formats::json::Value Serialize(KeyboardButtonPollType::PoolType poll_type,
                               formats::serialize::To<formats::json::Value>) {
  return formats::json::ValueBuilder(ToString(poll_type)).ExtractValue();
}

KeyboardButtonPollType Parse(const formats::json::Value& json,
                             formats::parse::To<KeyboardButtonPollType> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(
    const KeyboardButtonPollType& keyboard_button_poll_type,
    formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(keyboard_button_poll_type, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
