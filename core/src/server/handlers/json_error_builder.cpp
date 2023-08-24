#include <userver/server/handlers/json_error_builder.hpp>

#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>

#include <userver/server/handlers/exceptions.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers {

JsonErrorBuilder::JsonErrorBuilder(const CustomHandlerException& ex)
    : JsonErrorBuilder(
          ex.GetServiceCode().empty() ? GetFallbackServiceCode(ex.GetCode())
                                      : ex.GetServiceCode(),
          ex.what(),
          ex.GetExternalErrorBody().empty() ? GetCodeDescription(ex.GetCode())
                                            : ex.GetExternalErrorBody(),
          ex.GetDetails()) {}

JsonErrorBuilder::JsonErrorBuilder(std::string_view error_code,
                                   std::string internal_message,
                                   std::string_view external_error_body,
                                   formats::json::Value details)
    : internal_message_(std::move(internal_message)) {
  UASSERT_MSG(!error_code.empty(),
              "Service-specific error code must be provided");

  formats::json::ValueBuilder response_json(formats::json::Type::kObject);
  response_json["code"] = error_code;
  response_json["message"] = external_error_body;

  if (details.IsObject()) {
    response_json["details"] = std::move(details);
  }

  json_error_body_ = formats::json::ToString(response_json.ExtractValue());
}

}  // namespace server::handlers

USERVER_NAMESPACE_END
