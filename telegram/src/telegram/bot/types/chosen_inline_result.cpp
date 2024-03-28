#include <userver/telegram/bot/types/chosen_inline_result.hpp>

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
ChosenInlineResult Parse(const Value& data,
                         formats::parse::To<ChosenInlineResult>) {
  return ChosenInlineResult{
    data["result_id"].template As<std::string>(),
    data["from"].template As<std::unique_ptr<User>>(),
    data["location"].template As<std::unique_ptr<Location>>(),
    data["inline_message_id"].template As<std::optional<std::string>>(),
    data["query"].template As<std::string>()
  };
}

template <class Value>
Value Serialize(const ChosenInlineResult& chosen_inline_result,
                formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["result_id"] = chosen_inline_result.result_id;
  builder["from"] = chosen_inline_result.from;
  SetIfNotNull(builder, "location", chosen_inline_result.location);
  SetIfNotNull(builder, "inline_message_id", chosen_inline_result.inline_message_id);
  builder["query"] = chosen_inline_result.query;
  return builder.ExtractValue();
}

}  // namespace impl

ChosenInlineResult Parse(const formats::json::Value& json,
                         formats::parse::To<ChosenInlineResult> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const ChosenInlineResult& chosen_inline_result,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(chosen_inline_result, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
