#include <userver/telegram/bot/types/update.hpp>

#include <telegram/bot/formats/parse.hpp>
#include <telegram/bot/formats/serialize.hpp>
#include <telegram/bot/formats/value_builder.hpp>

#include <userver/formats/json.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

namespace impl {

template <class Value>
Update Parse(const Value& data, formats::parse::To<Update>) {
  return Update{
    data["update_id"].template As<std::int64_t>(),
    data["message"].template As<std::unique_ptr<Message>>(),
    data["edited_message"].template As<std::unique_ptr<Message>>(),
    data["channel_post"].template As<std::unique_ptr<Message>>(),
    data["edited_channel_post"].template As<std::unique_ptr<Message>>(),
    data["inline_query"].template As<std::unique_ptr<InlineQuery>>(),
    data["chosen_inline_result"].template As<std::unique_ptr<ChosenInlineResult>>(),
    data["callback_query"].template As<std::unique_ptr<CallbackQuery>>(),
    data["shipping_query"].template As<std::unique_ptr<ShippingQuery>>(),
    data["pre_checkout_query"].template As<std::unique_ptr<PreCheckoutQuery>>(),
    data["poll"].template As<std::unique_ptr<Poll>>(),
    data["poll_answer"].template As<std::unique_ptr<PollAnswer>>(),
    data["my_chat_member"].template As<std::unique_ptr<ChatMemberUpdated>>(),
    data["chat_member"].template As<std::unique_ptr<ChatMemberUpdated>>(),
    data["chat_join_request"].template As<std::unique_ptr<ChatJoinRequest>>()
  };
}

template <class Value>
Value Serialize(const Update& update, formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["update_id"] = update.update_id;
  SetIfNotNull(builder, "message", update.message);
  SetIfNotNull(builder, "edited_message", update.edited_message);
  SetIfNotNull(builder, "channel_post", update.channel_post);
  SetIfNotNull(builder, "edited_channel_post", update.edited_channel_post);
  SetIfNotNull(builder, "inline_query", update.inline_query);
  SetIfNotNull(builder, "chosen_inline_result", update.chosen_inline_result);
  SetIfNotNull(builder, "callback_query", update.callback_query);
  SetIfNotNull(builder, "shipping_query", update.shipping_query);
  SetIfNotNull(builder, "pre_checkout_query", update.pre_checkout_query);
  SetIfNotNull(builder, "poll", update.poll);
  SetIfNotNull(builder, "poll_answer", update.poll_answer);
  SetIfNotNull(builder, "my_chat_member", update.my_chat_member);
  SetIfNotNull(builder, "chat_member", update.chat_member);
  SetIfNotNull(builder, "chat_join_request", update.chat_join_request);
  return builder.ExtractValue();
}

}  // namespace impl

Update Parse(const formats::json::Value& json,
             formats::parse::To<Update> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const Update& update,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(update, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
