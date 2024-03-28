#include <userver/telegram/bot/requests/get_file.hpp>

#include <telegram/bot/requests/request_data.hpp>

#include <userver/formats/json.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

namespace impl {

template <class Value>
GetFileMethod::Parameters Parse(
    const Value& data,
    formats::parse::To<GetFileMethod::Parameters>) {
  GetFileMethod::Parameters parameters{
    data["file_id"].template As<std::string>()
  };
  return parameters;
}

template <class Value>
Value Serialize(const GetFileMethod::Parameters& parameters,
                formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["file_id"] = parameters.file_id;
  return builder.ExtractValue();
}

}  // namespace impl

GetFileMethod::Parameters::Parameters(std::string _file_id)
  : file_id(std::move(_file_id)) {}

void GetFileMethod::FillRequestData(clients::http::Request& request,
                                    const Parameters& parameters) {
  FillRequestDataAsJson<GetFileMethod>(request, parameters);
}

GetFileMethod::Reply GetFileMethod::ParseResponseData(
    clients::http::Response& response) {
  return ParseResponseDataFromJson<GetFileMethod>(response);
}

GetFileMethod::Parameters Parse(
    const formats::json::Value& json,
    formats::parse::To<GetFileMethod::Parameters> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(
    const GetFileMethod::Parameters& parameters,
    formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(parameters, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
