#include <userver/telegram/bot/requests/get_chat.hpp>

#include <telegram/bot/formats/parse.hpp>
#include <telegram/bot/formats/serialize.hpp>
#include <telegram/bot/requests/request_data.hpp>

#include <userver/formats/json.hpp>
#include <userver/formats/json/serialize_variant.hpp>
#include <userver/formats/parse/variant.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

namespace impl {

template <class Value>
GetChatMethod::Parameters Parse(
    const Value& data,
    formats::parse::To<GetChatMethod::Parameters>) {
  GetChatMethod::Parameters parameters{
    data["chat_id"].template As<ChatId>()
  };
  return parameters;
}

template <class Value>
Value Serialize(const GetChatMethod::Parameters& parameters,
                formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["chat_id"] = parameters.chat_id;
  return builder.ExtractValue();
}

}  // namespace impl

GetChatMethod::Parameters::Parameters(ChatId _chat_id)
  : chat_id(std::move(_chat_id)) {}

void GetChatMethod::FillRequestData(clients::http::Request& request,
                                    const Parameters& parameters) {
  FillRequestDataAsJson<GetChatMethod>(request, parameters);
}

GetChatMethod::Reply GetChatMethod::ParseResponseData(
    clients::http::Response& response) {
  return ParseResponseDataFromJson<GetChatMethod>(response);
}

GetChatMethod::Parameters Parse(
    const formats::json::Value& json,
    formats::parse::To<GetChatMethod::Parameters> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(
    const GetChatMethod::Parameters& parameters,
    formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(parameters, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
