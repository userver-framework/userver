#include "errors.hpp"

namespace real_medium::utils::error {


ErrorBuilder::ErrorBuilder(std::string_view field, std::string_view msg) {
  json_error_body_ = userver::formats::json::ToString(MakeError(field, msg));
}

std::string ErrorBuilder::GetExternalBody() const { return json_error_body_; }

userver::formats::json::Value ValidationException::ToJson() const {
  return userver::formats::json::FromString(GetExternalErrorBody());
}

}  // namespace real_medium::errors
