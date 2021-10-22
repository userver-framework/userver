#include <userver/server/handlers/legacy_json_error_builder.hpp>

#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>

#include <userver/server/handlers/exceptions.hpp>
#include <userver/server/http/http_error.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers {

LegacyJsonErrorBuilder::LegacyJsonErrorBuilder(const CustomHandlerException& ex)
    : LegacyJsonErrorBuilder(http::GetHttpStatus(ex.GetCode()), ex.what(),
                             ex.GetExternalErrorBody(), ex.GetDetails()) {}

LegacyJsonErrorBuilder::LegacyJsonErrorBuilder(http::HttpStatus status,
                                               std::string internal_message,
                                               std::string external_error_body)
    : LegacyJsonErrorBuilder(status, std::move(internal_message),
                             std::move(external_error_body),
                             formats::json::Value{}) {}

LegacyJsonErrorBuilder::LegacyJsonErrorBuilder(
    http::HttpStatus status, std::string internal_message,
    std::string external_error_body, const formats::json::Value& details)
    : internal_message_(std::move(internal_message)) {
  formats::json::ValueBuilder response_json(formats::json::Type::kObject);

  response_json["code"] = std::to_string(static_cast<int>(status));

  if (!external_error_body.empty()) {
    response_json["message"] = external_error_body;
  } else {
    response_json["message"] = HttpStatusString(status);
  }

  if (details.IsObject()) {
    response_json["details"] = details;
  }

  json_error_body_ = formats::json::ToString(response_json.ExtractValue());
}

static_assert(impl::kHasInternalMessage<LegacyJsonErrorBuilder>);

}  // namespace server::handlers

USERVER_NAMESPACE_END
