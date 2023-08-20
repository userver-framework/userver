#include "article_validators.hpp"

#include "length_validator.hpp"

namespace real_medium::validator {

void ValidateTitle(const std::string& title) {
  static constexpr std::size_t MIN_TITLE_LEN = 3;
  static constexpr std::size_t MAX_TITLE_LEN = 256;
  CheckLength(title, "title", MIN_TITLE_LEN, MAX_TITLE_LEN);
}
void ValidateDescription(const std::string& description) {
  static constexpr std::size_t MIN_DESCR_LEN = 5;
  static constexpr std::size_t MAX_DESCR_LEN = 8192;

  CheckLength(description, "description", MIN_DESCR_LEN, MAX_DESCR_LEN);
}
void ValidateBody(const std::string& body) {
  static constexpr std::size_t MIN_BODY_LEN = 5;
  static constexpr std::size_t MAX_BODY_LEN = 65535;

  CheckLength(body, "body", MIN_BODY_LEN, MAX_BODY_LEN);
}
void ValidateTags(const std::vector<std::string>& tags) {
  static constexpr std::size_t MIN_TAG_NAME_LEN = 2;
  static constexpr std::size_t MAX_TAG_NAME_LEN = 256;

  for(const auto &tag : tags) {
    CheckLength(tag, "tagList", MIN_TAG_NAME_LEN, MAX_TAG_NAME_LEN);
  }
}

}  // namespace real_medium::validator