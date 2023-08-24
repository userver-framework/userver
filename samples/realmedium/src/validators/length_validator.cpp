#include "length_validator.hpp"

#include "utils/make_error.hpp"

namespace real_medium::validator {

void CheckLength(const std::string& value, std::string_view fieldName,
                 std::size_t minLen, std::size_t maxLen) {
  auto length = userver::utils::text::utf8::GetCodePointsCount(value);
  if (length < minLen || length > maxLen) {
    throw utils::error::ValidationException{
        fieldName,
        fmt::format("must be longer than {} characters and less than {}",
                    minLen, maxLen)};
  }
}

}  // namespace real_medium::validator
