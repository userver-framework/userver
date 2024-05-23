#include <userver/telegram/bot/requests/get_me.hpp>

#include <telegram/bot/requests/request_data.hpp>

#include <userver/formats/json.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

namespace impl {

template <class Value>
GetMeMethod::Parameters Parse(const Value&,
  formats::parse::To<GetMeMethod::Parameters>) {
  return GetMeMethod::Parameters{};
}

template <class Value>
Value Serialize(const GetMeMethod::Parameters&, formats::serialize::To<Value>) {
  typename Value::Builder builder;
  return builder.ExtractValue();
}

}  // namespace impl

void GetMeMethod::FillRequestData(clients::http::Request& request,
                                  const Parameters& parameters) {
  FillRequestDataAsJson<GetMeMethod>(request, parameters);
}

GetMeMethod::Reply GetMeMethod::ParseResponseData(
    clients::http::Response& response) {
  return ParseResponseDataFromJson<GetMeMethod>(response);
}

GetMeMethod::Parameters Parse(const formats::json::Value& json,
                              formats::parse::To<GetMeMethod::Parameters> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const GetMeMethod::Parameters& parameters,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(parameters, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
