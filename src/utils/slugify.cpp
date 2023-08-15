#include <memory>
#include <string>
#include "slugify.hpp"
#include "errors.hpp"
#include "fmt/core.h"
#include "unicode/translit.h"
#include "userver/formats/json/value.hpp"

namespace real_medium::utils::slug {

std::string Slugify(const std::string& str) {
  UErrorCode status{U_ZERO_ERROR};
  UParseError parse_error{};
  const auto transliterator =
      std::unique_ptr<icu::Transliterator>{icu::Transliterator::createFromRules(
          "Slugify",
          ":: Any-Latin;"
          ":: [:Nonspacing Mark:] Remove;"
          ":: [:Punctuation:] Remove;"
          ":: [:Symbol:] Remove;"
          ":: Latin-ASCII;"
          ":: Lower();"
          "' ' {' '} > ;"
          "::NULL;"
          "[:Separator:] > '-'",
          UTRANS_FORWARD, parse_error, status)};
      if (status != U_ZERO_ERROR) {
        throw std::runtime_error(fmt::format(
            "icu::Transliterator::createFromRules failed with status {}", status));
      }
      auto unicode_str = icu::UnicodeString::fromUTF8(str);
      transliterator->transliterate(unicode_str);
      std::string slug;
      return unicode_str.toUTF8String(slug);
}

}  // namespace real_medium::slug
