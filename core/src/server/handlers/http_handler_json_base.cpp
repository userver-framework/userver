#include <server/handlers/http_handler_json_base.hpp>

#include <formats/json/exception.hpp>
#include <formats/json/serialize.hpp>

#include <server/handlers/exceptions.hpp>
#include <server/handlers/json_error_builder.hpp>
#include <server/http/content_type.hpp>
#include <server/http/http_error.hpp>

namespace server {
namespace handlers {

namespace {

const std::string kRequestDataName = "__request_json";
const std::string kResponseDataName = "__response_json";

}  // namespace

HttpHandlerJsonBase::HttpHandlerJsonBase(
    const components::ComponentConfig& config,
    const components::ComponentContext& component_context)
    : HttpHandlerBase(config, component_context) {}

std::string HttpHandlerJsonBase::HandleRequestThrow(
    const http::HttpRequest& request, request::RequestContext& context) const {
  const auto& request_json =
      context.GetData<const formats::json::Value&>(kRequestDataName);

  auto& response = request.GetHttpResponse();
  response.SetContentType(http::content_type::kApplicationJson);

  try {
    const auto& response_json = context.SetData<const formats::json::Value>(
        kResponseDataName,
        HandleRequestJsonThrow(request, request_json, context));
    return formats::json::ToString(response_json);
  } catch (const http::HttpException& ex) {
    // TODO Remove this catch branch
    formats::json::ValueBuilder response_json(formats::json::Type::kObject);

    auto status = ex.GetStatus();
    response_json["code"] = std::to_string(static_cast<int>(status));

    const auto& error_message = ex.GetExternalErrorBody();
    if (!error_message.empty()) {
      response_json["message"] = error_message;
    } else {
      response_json["message"] = HttpStatusString(status);
    }

    throw http::HttpException(
        ex.GetStatus(), ex.what(),
        formats::json::ToString(response_json.ExtractValue()));
  }
}

const formats::json::Value* HttpHandlerJsonBase::GetRequestJson(
    const request::RequestContext& context) const {
  return context.GetDataOptional<const formats::json::Value>(kRequestDataName);
}

const formats::json::Value* HttpHandlerJsonBase::GetResponseJson(
    const request::RequestContext& context) const {
  return context.GetDataOptional<const formats::json::Value>(kResponseDataName);
}

std::string HttpHandlerJsonBase::GetFormattedExternalErrorBody(
    http::HttpStatus status, std::string external_error_body) const {
  return JsonErrorBuilder(status, {}, std::move(external_error_body))
      .GetExternalBody();
}

void HttpHandlerJsonBase::ParseRequestData(
    const http::HttpRequest& request, request::RequestContext& context) const {
  formats::json::Value request_json;
  try {
    if (!request.RequestBody().empty())
      request_json = formats::json::FromString(request.RequestBody());
  } catch (const formats::json::Exception& e) {
    throw RequestParseError(
        InternalMessage{"Invalid JSON body"},
        ExternalBody{std::string("Invalid JSON body: ") + e.what()});
  }

  context.SetData<const formats::json::Value>(kRequestDataName, request_json);
}

}  // namespace handlers
}  // namespace server
