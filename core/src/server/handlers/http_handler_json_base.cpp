#include <server/handlers/http_handler_json_base.hpp>

#include <formats/json/exception.hpp>
#include <formats/json/serialize.hpp>

#include <server/handlers/exceptions.hpp>
#include <server/http/content_type.hpp>
#include <server/http/http_error.hpp>

namespace server {
namespace handlers {

namespace {

class JsonErrorBuilder {
 public:
  explicit JsonErrorBuilder(const CustomHandlerException& ex)
      : internal_message{ex.what()} {
    formats::json::ValueBuilder response_json(formats::json::Type::kObject);

    auto status = http::GetHttpStatus(ex.GetCode());
    response_json["code"] = static_cast<int>(status);
    response_json["error"] = HttpStatusString(status);

    const auto& error_message = ex.GetExternalErrorBody();
    if (!error_message.empty()) response_json["message"] = error_message;
    json_error_body = formats::json::ToString(response_json.ExtractValue());
  }

  const std::string& GetInternalMessage() const { return internal_message; };
  const std::string& GetExternalBody() const { return json_error_body; }

 private:
  std::string internal_message;
  std::string json_error_body;
};

static_assert(detail::kHasInternalMessage<JsonErrorBuilder>, "");

}  // namespace

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
  } catch (const formats::json::Exception& e) {
    throw RequestParseError(
        InternalMessage{"Invalid JSON body"},
        ExternalBody{std::string("Invalid JSON body: ") + e.what()});
  }

  auto& response = request.GetHttpResponse();
  response.SetContentType(http::content_type::kApplicationJson);

  try {
    auto response_json = HandleRequestJsonThrow(request, request_json, context);
    return formats::json::ToString(response_json);
  } catch (const http::HttpException& ex) {
    // TODO Remove this catch branch
    formats::json::ValueBuilder response_json(formats::json::Type::kObject);

    auto status = ex.GetStatus();
    response_json["code"] = static_cast<int>(status);
    response_json["error"] = HttpStatusString(status);

    const auto& error_message = ex.GetExternalErrorBody();
    if (!error_message.empty()) response_json["message"] = error_message;

    throw http::HttpException(
        ex.GetStatus(), ex.what(),
        formats::json::ToString(response_json.ExtractValue()));
  } catch (const CustomHandlerException& ex) {
    throw CustomHandlerException(JsonErrorBuilder{ex}, ex.GetCode());
  }
}

}  // namespace handlers
}  // namespace server
