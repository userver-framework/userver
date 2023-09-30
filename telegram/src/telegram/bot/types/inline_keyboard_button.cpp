#include <userver/telegram/bot/types/inline_keyboard_button.hpp>

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
InlineKeyboardButton Parse(const Value& data,
                           formats::parse::To<InlineKeyboardButton>) {
  return InlineKeyboardButton{
    data["text"].template As<std::string>(),
    data["url"].template As<std::optional<std::string>>(),
    data["callback_data"].template As<std::optional<std::string>>(),
    data["web_app"].template As<std::unique_ptr<WebAppInfo>>(),
    data["login_url"].template As<std::unique_ptr<LoginUrl>>(),
    data["switch_inline_query"].template As<std::optional<std::string>>(),
    data["switch_inline_query_current_chat"].template As<std::optional<std::string>>(),
    data["switch_inline_query_chosen_chat"].template As<std::unique_ptr<SwitchInlineQueryChosenChat>>(),
    data["callback_game"].template As<std::unique_ptr<CallbackGame>>(),
    data["pay"].template As<std::optional<bool>>()
  };
}

template <class Value>
Value Serialize(const InlineKeyboardButton& inline_keyboard_button,
                formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["text"] = inline_keyboard_button.text;
  SetIfNotNull(builder, "url", inline_keyboard_button.url);
  SetIfNotNull(builder, "callback_data", inline_keyboard_button.callback_data);
  SetIfNotNull(builder, "web_app", inline_keyboard_button.web_app);
  SetIfNotNull(builder, "login_url", inline_keyboard_button.login_url);
  SetIfNotNull(builder, "switch_inline_query", inline_keyboard_button.switch_inline_query);
  SetIfNotNull(builder, "switch_inline_query_current_chat", inline_keyboard_button.switch_inline_query_current_chat);
  SetIfNotNull(builder, "switch_inline_query_chosen_chat", inline_keyboard_button.switch_inline_query_chosen_chat);
  SetIfNotNull(builder, "callback_game", inline_keyboard_button.callback_game);
  SetIfNotNull(builder, "pay", inline_keyboard_button.pay);
  return builder.ExtractValue();
}

}  // namespace impl

InlineKeyboardButton Parse(const formats::json::Value& json,
                           formats::parse::To<InlineKeyboardButton> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const InlineKeyboardButton& inline_keyboard_button,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(inline_keyboard_button, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
