#include <userver/telegram/bot/requests/copy_message.hpp>

#include <telegram/bot/formats/parse.hpp>
#include <telegram/bot/formats/serialize.hpp>
#include <telegram/bot/formats/value_builder.hpp>
#include <telegram/bot/requests/request_data.hpp>

#include <userver/formats/json.hpp>
#include <userver/formats/json/serialize_variant.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/parse/variant.hpp>
#include <userver/formats/serialize/common_containers.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

namespace impl {

template <class Value>
CopyMessageMethod::Parameters Parse(
    const Value& data,
    formats::parse::To<CopyMessageMethod::Parameters>) {
  CopyMessageMethod::Parameters parameters{
    data["chat_id"].template As<ChatId>(),
    data["from_chat_id"].template As<ChatId>(),
    data["message_id"].template As<std::int64_t>()
  };
  parameters.message_thread_id = data["message_thread_id"].template As<std::optional<std::int64_t>>();
  parameters.caption = data["caption"].template As<std::optional<std::string>>();
  parameters.parse_mode = data["parse_mode"].template As<std::optional<std::string>>();
  parameters.caption_entities = data["caption_entities"].template As<std::optional<std::vector<MessageEntity>>>();
  parameters.disable_notification = data["disable_notification"].template As<std::optional<bool>>();
  parameters.protect_content = data["protect_content"].template As<std::optional<bool>>();
  parameters.reply_to_message_id = data["reply_to_message_id"].template As<std::optional<std::int64_t>>();
  parameters.allow_sending_without_reply = data["allow_sending_without_reply"].template As<std::optional<bool>>();
  parameters.reply_markup = data["reply_markup"].template As<std::optional<ReplyMarkup>>();
  return parameters;
}

template <class Value>
Value Serialize(const CopyMessageMethod::Parameters& parameters,
                formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["chat_id"] = parameters.chat_id;
  SetIfNotNull(builder, "message_thread_id", parameters.message_thread_id);
  builder["from_chat_id"] = parameters.from_chat_id;
  builder["message_id"] = parameters.message_id;
  SetIfNotNull(builder, "caption", parameters.caption);
  SetIfNotNull(builder, "parse_mode", parameters.parse_mode);
  SetIfNotNull(builder, "caption_entities", parameters.caption_entities);
  SetIfNotNull(builder, "disable_notification", parameters.disable_notification);
  SetIfNotNull(builder, "protect_content", parameters.protect_content);
  SetIfNotNull(builder, "reply_to_message_id", parameters.reply_to_message_id);
  SetIfNotNull(builder, "allow_sending_without_reply", parameters.allow_sending_without_reply);
  SetIfNotNull(builder, "reply_markup", parameters.reply_markup);
  return builder.ExtractValue();
}

}  // namespace impl

CopyMessageMethod::Parameters::Parameters(ChatId _chat_id,
                                          ChatId _from_chat_id,
                                          std::int64_t _message_id)
  : chat_id(std::move(_chat_id))
  , from_chat_id(std::move(_from_chat_id))
  , message_id(_message_id) {}

void CopyMessageMethod::FillRequestData(clients::http::Request& request,
                                        const Parameters& parameters) {
  FillRequestDataAsJson<CopyMessageMethod>(request, parameters);
}

CopyMessageMethod::Reply CopyMessageMethod::ParseResponseData(
    clients::http::Response& response) {
  return ParseResponseDataFromJson<CopyMessageMethod>(response);
}

CopyMessageMethod::Parameters Parse(
    const formats::json::Value& json,
    formats::parse::To<CopyMessageMethod::Parameters> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(
    const CopyMessageMethod::Parameters& parameters,
    formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(parameters, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
