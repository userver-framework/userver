#include <userver/telegram/bot/types/inline_query.hpp>

#include <telegram/bot/formats/parse.hpp>
#include <telegram/bot/formats/serialize.hpp>
#include <telegram/bot/formats/value_builder.hpp>

#include <userver/formats/json.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/utils/trivial_map.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

namespace {

constexpr utils::TrivialBiMap kChatTypeMap([](auto selector) {
  return selector()
    .Case(InlineQuery::ChatType::kPrivate, "private")
    .Case(InlineQuery::ChatType::kGroup, "group")
    .Case(InlineQuery::ChatType::kSupergroup, "supergroup")
    .Case(InlineQuery::ChatType::kChannel, "channel")
    .Case(InlineQuery::ChatType::kSender, "sender");
});

}  // namespace

namespace impl {

template <class Value>
InlineQuery Parse(const Value& data, formats::parse::To<InlineQuery>) {
  return InlineQuery{
    data["id"].template As<std::string>(),
    data["from"].template As<std::unique_ptr<User>>(),
    data["query"].template As<std::string>(),
    data["offset"].template As<std::string>(),
    data["chat_type"].template As<std::optional<InlineQuery::ChatType>>(),
    data["location"].template As<std::unique_ptr<Location>>()
  };
}

template <class Value>
Value Serialize(const InlineQuery& inline_query, formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["id"] = inline_query.id;
  builder["from"] = inline_query.from;
  builder["query"] = inline_query.query;
  builder["offset"] = inline_query.offset;
  SetIfNotNull(builder, "chat_type", inline_query.chat_type);
  SetIfNotNull(builder, "location", inline_query.location);
  return builder.ExtractValue();
}

}  // namespace impl

std::string_view ToString(InlineQuery::ChatType chat_type) {
  return utils::impl::EnumToStringView(chat_type, kChatTypeMap);
}

InlineQuery::ChatType Parse(const formats::json::Value& value,
                            formats::parse::To<InlineQuery::ChatType>) {
  return utils::ParseFromValueString(value, kChatTypeMap);
}

formats::json::Value Serialize(InlineQuery::ChatType chat_type,
                               formats::serialize::To<formats::json::Value>) {
  return formats::json::ValueBuilder(ToString(chat_type)).ExtractValue();
}

InlineQuery Parse(const formats::json::Value& json,
                  formats::parse::To<InlineQuery> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const InlineQuery& inline_query,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(inline_query, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
