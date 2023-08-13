#include "length_validator.hpp"

namespace real_medium::validators {

const std::string CheckLength(const std::string& value, std::string_view fieldName,
                            int minLen, int maxLen) {
  const auto length =  userver::utils::text::utf8::GetCodePointsCount(value);
  if (length > minLen && length < maxLen)
    return value;

  throw real_medium::utils::error::ValidationException{
    real_medium::utils::error::ErrorBuilder{fieldName, fmt::format("must be longer than {} characters and less than {}", minLen, maxLen)}};
}

const std::string CheckLength(const userver::formats::json::Value& json,
                            std::string_view fieldName, int minLen, int maxLen) {
  if (!json.HasMember(fieldName))
    throw real_medium::utils::error::ValidationException{real_medium::utils::error::ErrorBuilder{fieldName, "required"}};
  const auto& str = json[fieldName].As<std::string>();
  return CheckLength(str, fieldName, minLen, maxLen);
}

}  // namespace real_medium::validators
