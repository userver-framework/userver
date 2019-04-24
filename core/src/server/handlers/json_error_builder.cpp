#include <server/handlers/json_error_builder.hpp>

#include <formats/json/serialize.hpp>
#include <formats/json/value_builder.hpp>

#include <server/handlers/exceptions.hpp>
#include <server/http/http_error.hpp>

namespace server {
namespace handlers {

JsonErrorBuilder::JsonErrorBuilder(const CustomHandlerException& ex)
    : internal_message{ex.what()} {
  formats::json::ValueBuilder response_json(formats::json::Type::kObject);

  const auto status = http::GetHttpStatus(ex.GetCode());
  response_json["code"] = std::to_string(static_cast<int>(status));

  const auto& error_message = ex.GetExternalErrorBody();
  if (!error_message.empty()) {
    response_json["message"] = error_message;
  } else {
    response_json["message"] = HttpStatusString(status);
  }

  const auto& details = ex.GetDetails();
  if (details.IsObject()) {
    response_json["details"] = details;
  }

  json_error_body = formats::json::ToString(response_json.ExtractValue());
}

static_assert(detail::kHasInternalMessage<JsonErrorBuilder>);

}  // namespace handlers
}  // namespace server
