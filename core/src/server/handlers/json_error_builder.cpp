#include <server/handlers/json_error_builder.hpp>

#include <formats/json/serialize.hpp>
#include <formats/json/value_builder.hpp>

#include <server/handlers/exceptions.hpp>
#include <server/http/http_error.hpp>

namespace server {
namespace handlers {

JsonErrorBuilder::JsonErrorBuilder(const CustomHandlerException& ex)
    : JsonErrorBuilder(http::GetHttpStatus(ex.GetCode()), ex.what(),
                       ex.GetExternalErrorBody(), ex.GetDetails()) {}

JsonErrorBuilder::JsonErrorBuilder(
    http::HttpStatus status, std::string internal_message,
    std::string external_error_body,
    boost::optional<const formats::json::Value&> details)
    : internal_message_(std::move(internal_message)) {
  formats::json::ValueBuilder response_json(formats::json::Type::kObject);

  response_json["code"] = std::to_string(static_cast<int>(status));

  if (!external_error_body.empty()) {
    response_json["message"] = external_error_body;
  } else {
    response_json["message"] = HttpStatusString(status);
  }

  if (details && details->IsObject()) {
    response_json["details"] = *details;
  }

  json_error_body_ = formats::json::ToString(response_json.ExtractValue());
}

static_assert(detail::kHasInternalMessage<JsonErrorBuilder>);

}  // namespace handlers
}  // namespace server
