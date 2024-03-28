#include <userver/telegram/bot/requests/forward_message.hpp>

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
ForwardMessageMethod::Parameters Parse(
    const Value& data,
    formats::parse::To<ForwardMessageMethod::Parameters>) {
  ForwardMessageMethod::Parameters parameters{
    data["chat_id"].template As<ChatId>(),
    data["from_chat_id"].template As<ChatId>(),
    data["message_id"].template As<std::int64_t>()
  };
  parameters.message_thread_id = data["message_thread_id"].template As<std::optional<std::int64_t>>();
  parameters.disable_notification = data["disable_notification"].template As<std::optional<bool>>();
  parameters.protect_content = data["protect_content"].template As<std::optional<bool>>();
  return parameters;
}

template <class Value>
Value Serialize(const ForwardMessageMethod::Parameters& parameters,
                formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["chat_id"] = parameters.chat_id;
  SetIfNotNull(builder, "message_thread_id", parameters.message_thread_id);
  builder["from_chat_id"] = parameters.from_chat_id;
  SetIfNotNull(builder, "disable_notification", parameters.disable_notification);
  SetIfNotNull(builder, "protect_content", parameters.protect_content);
  builder["message_id"] = parameters.message_id;
  return builder.ExtractValue();
}

}  // namespace impl

ForwardMessageMethod::Parameters::Parameters(ChatId _chat_id,
                                             ChatId _from_chat_id,
                                             std::int64_t _message_id)
  : chat_id(std::move(_chat_id))
  , from_chat_id(std::move(_from_chat_id))
  , message_id(_message_id) {}

void ForwardMessageMethod::FillRequestData(clients::http::Request& request,
                                           const Parameters& parameters) {
  FillRequestDataAsJson<ForwardMessageMethod>(request, parameters);
}

ForwardMessageMethod::Reply ForwardMessageMethod::ParseResponseData(
    clients::http::Response& response) {
  return ParseResponseDataFromJson<ForwardMessageMethod>(response);
}

ForwardMessageMethod::Parameters Parse(
    const formats::json::Value& json,
    formats::parse::To<ForwardMessageMethod::Parameters> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(
    const ForwardMessageMethod::Parameters& parameters,
    formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(parameters, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
