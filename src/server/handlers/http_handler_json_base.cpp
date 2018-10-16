#include <server/handlers/http_handler_json_base.hpp>

#include <formats/json/exception.hpp>
#include <formats/json/serialize.hpp>

#include <server/http/content_type.hpp>
#include <server/http/http_error.hpp>

namespace server {
namespace handlers {

HttpHandlerJsonBase::HttpHandlerJsonBase(
    const components::ComponentConfig& config,
    const components::ComponentContext& component_context)
    : HttpHandlerBase(config, component_context) {}

std::string HttpHandlerJsonBase::HandleRequestThrow(
    const http::HttpRequest& request, request::RequestContext& context) const {
  formats::json::Value request_json;
  try {
    if (!request.RequestBody().empty())
      request_json = formats::json::FromString(request.RequestBody());
  } catch (const formats::json::JsonException& e) {
    throw http::BadRequest("Invalid JSON body",
                           std::string("Invalid JSON body: ") + e.what());
  }

  auto& response = request.GetHttpResponse();
  response.SetContentType(http::content_type::kApplicationJson);

  try {
    auto response_json = HandleRequestJsonThrow(request, request_json, context);
    return formats::json::ToString(response_json);
  } catch (const http::HttpException& ex) {
    formats::json::ValueBuilder response_json(formats::json::Type::kObject);

    const auto& error = ex.GetExternalErrorBody();
    if (!error.empty()) response_json["error"] = error;

    throw http::HttpException(
        ex.GetStatus(), ex.what(),
        formats::json::ToString(response_json.ExtractValue()));
  }
}

}  // namespace handlers
}  // namespace server
