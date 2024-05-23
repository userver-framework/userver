#include <userver/telegram/bot/requests/send_contact.hpp>

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
SendContactMethod::Parameters Parse(
    const Value& data,
    formats::parse::To<SendContactMethod::Parameters>) {
  SendContactMethod::Parameters parameters{
    data["chat_id"].template As<ChatId>(),
    data["phone_number"].template As<std::string>(),
    data["first_name"].template As<std::string>()
  };
  parameters.message_thread_id = data["message_thread_id"].template As<std::optional<std::int64_t>>();
  parameters.last_name = data["last_name"].template As<std::optional<std::string>>();
  parameters.vcard = data["vcard"].template As<std::optional<std::string>>();
  parameters.disable_notification = data["disable_notification"].template As<std::optional<bool>>();
  parameters.protect_content = data["protect_content"].template As<std::optional<bool>>();
  parameters.reply_to_message_id = data["reply_to_message_id"].template As<std::optional<std::int64_t>>();
  parameters.allow_sending_without_reply = data["allow_sending_without_reply"].template As<std::optional<bool>>();
  parameters.reply_markup = data["reply_markup"].template As<std::optional<ReplyMarkup>>();
  return parameters;
}

template <class Value>
Value Serialize(const SendContactMethod::Parameters& parameters, formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["chat_id"] = parameters.chat_id;
  SetIfNotNull(builder, "message_thread_id", parameters.message_thread_id);
  builder["phone_number"] = parameters.phone_number;
  builder["first_name"] = parameters.first_name;
  SetIfNotNull(builder, "last_name", parameters.last_name);
  SetIfNotNull(builder, "vcard", parameters.vcard);
  SetIfNotNull(builder, "disable_notification", parameters.disable_notification);
  SetIfNotNull(builder, "protect_content", parameters.protect_content);
  SetIfNotNull(builder, "reply_to_message_id", parameters.reply_to_message_id);
  SetIfNotNull(builder, "allow_sending_without_reply", parameters.allow_sending_without_reply);
  SetIfNotNull(builder, "reply_markup", parameters.reply_markup);
  return builder.ExtractValue();
}

}  // namespace impl

SendContactMethod::Parameters::Parameters(ChatId _chat_id,
                                          std::string _phone_number,
                                          std::string _first_name)
  : chat_id(std::move(_chat_id))
  , phone_number(std::move(_phone_number))
  , first_name(std::move(_first_name)) {}

void SendContactMethod::FillRequestData(clients::http::Request& request,
                                        const Parameters& parameters) {
  FillRequestDataAsJson<SendContactMethod>(request, parameters);
}

SendContactMethod::Reply SendContactMethod::ParseResponseData(
    clients::http::Response& response) {
  return ParseResponseDataFromJson<SendContactMethod>(response);
}

SendContactMethod::Parameters Parse(
    const formats::json::Value& json,
    formats::parse::To<SendContactMethod::Parameters> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(
    const SendContactMethod::Parameters& parameters,
    formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(parameters, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
