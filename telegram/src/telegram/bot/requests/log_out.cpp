#include <userver/telegram/bot/requests/log_out.hpp>

#include <telegram/bot/requests/request_data.hpp>

#include <userver/formats/json.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

namespace impl {

template <class Value>
LogOutMethod::Parameters Parse(const Value&,
                               formats::parse::To<LogOutMethod::Parameters>) {
  return LogOutMethod::Parameters{};
}

template <class Value>
Value Serialize(const LogOutMethod::Parameters&,
                formats::serialize::To<Value>) {
  typename Value::Builder builder;
  return builder.ExtractValue();
}

}  // namespace impl

void LogOutMethod::FillRequestData(clients::http::Request& request,
                                   const Parameters& parameters) {
  FillRequestDataAsJson<LogOutMethod>(request, parameters);
}

LogOutMethod::Reply LogOutMethod::ParseResponseData(
    clients::http::Response& response) {
  return ParseResponseDataFromJson<LogOutMethod>(response);
}

LogOutMethod::Parameters Parse(
    const formats::json::Value& json,
    formats::parse::To<LogOutMethod::Parameters> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(
    const LogOutMethod::Parameters& parameters,
    formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(parameters, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
