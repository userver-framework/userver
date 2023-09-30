#include <userver/telegram/bot/types/switch_inline_query_chosen_chat.hpp>

#include <telegram/bot/formats/value_builder.hpp>

#include <userver/formats/json.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

namespace impl {

template <class Value>
SwitchInlineQueryChosenChat Parse(const Value& data,
                                  formats::parse::To<SwitchInlineQueryChosenChat>) {
  return SwitchInlineQueryChosenChat{
    data["query"].template As<std::optional<std::string>>(),
    data["allow_user_chats"].template As<std::optional<bool>>(),
    data["allow_bot_chats"].template As<std::optional<bool>>(),
    data["allow_group_chats"].template As<std::optional<bool>>(),
    data["allow_channel_chats"].template As<std::optional<bool>>()
  };
}

template <class Value>
Value Serialize(const SwitchInlineQueryChosenChat& switch_inline_query_chosen_chat,
                formats::serialize::To<Value>) {
  typename Value::Builder builder;
  SetIfNotNull(builder, "query", switch_inline_query_chosen_chat.query);
  SetIfNotNull(builder, "allow_user_chats", switch_inline_query_chosen_chat.allow_user_chats);
  SetIfNotNull(builder, "allow_bot_chats", switch_inline_query_chosen_chat.allow_bot_chats);
  SetIfNotNull(builder, "allow_group_chats", switch_inline_query_chosen_chat.allow_group_chats);
  SetIfNotNull(builder, "allow_channel_chats", switch_inline_query_chosen_chat.allow_channel_chats);
  return builder.ExtractValue();
}

}  // namespace impl

SwitchInlineQueryChosenChat Parse(const formats::json::Value& json,
                                  formats::parse::To<SwitchInlineQueryChosenChat> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const SwitchInlineQueryChosenChat& switch_inline_query_chosen_chat,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(switch_inline_query_chosen_chat, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
