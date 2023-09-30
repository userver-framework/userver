#include <userver/telegram/bot/requests/close.hpp>

#include <telegram/bot/requests/request_data.hpp>

#include <userver/formats/json.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

namespace impl {

template <class Value>
CloseMethod::Parameters Parse(const Value&,
                              formats::parse::To<CloseMethod::Parameters>) {
  return CloseMethod::Parameters{};
}

template <class Value>
Value Serialize(const CloseMethod::Parameters&, formats::serialize::To<Value>) {
  typename Value::Builder builder;
  return builder.ExtractValue();
}

}  // namespace impl

void CloseMethod::FillRequestData(clients::http::Request& request,
                                  const Parameters& parameters) {
  FillRequestDataAsJson<CloseMethod>(request, parameters);
}

CloseMethod::Reply CloseMethod::ParseResponseData(
    clients::http::Response& response) {
  return ParseResponseDataFromJson<CloseMethod>(response);
}

CloseMethod::Parameters Parse(const formats::json::Value& json,
                              formats::parse::To<CloseMethod::Parameters> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const CloseMethod::Parameters& parameters,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(parameters, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
