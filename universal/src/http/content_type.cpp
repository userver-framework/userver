#include <userver/http/content_type.hpp>

#include <cstdlib>
#include <functional>
#include <iostream>

#include <fmt/compile.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/functional/hash.hpp>

#include <userver/utils/assert.hpp>
#include <userver/utils/str_icase.hpp>

USERVER_NAMESPACE_BEGIN

namespace http {
namespace {

constexpr int kMaxQuality = 1000;
const std::string kDefaultCharset = "UTF-8";

const std::string kOwsChars = " \t";
const std::string kTypeTokenInvalidChars = kOwsChars + '/';
const std::string kCharsetParamName = "charset";
const std::string kQualityParamName = "q";

const std::string kTokenAny = "*";

std::string_view LtrimOws(std::string_view view) {
  const auto first_pchar_pos = view.find_first_not_of(kOwsChars);
  if (first_pchar_pos != std::string_view::npos) {
    view.remove_prefix(first_pchar_pos);
  }
  return view;
}

std::string_view RtrimOws(std::string_view view) {
  const auto last_pchar_pos = view.find_last_not_of(kOwsChars);
  if (last_pchar_pos != std::string_view::npos) {
    view.remove_suffix(view.size() - (last_pchar_pos + 1));
  }

  return view;
}

int ParseQuality(std::string_view param_value) {
  static constexpr size_t kFullPrecisionLength = 5;  // "1.000"

  if (!param_value.empty() && param_value.size() <= kFullPrecisionLength) {
    if (param_value[0] == '0') {
      if (param_value.size() == 1) return 0;
      if (param_value[1] == '.') {
        // FIXME: replace with from_chars
        int quality = 0;
        for (size_t i = 2; i < param_value.size(); ++i) {
          if (param_value[i] < '0' || param_value[i] > '9') {
            quality = -1;
            break;
          }
          quality *= 10;
          quality += param_value[i] - '0';
        }
        if (quality >= 0) {
          for (auto i = param_value.size(); i < kFullPrecisionLength; ++i) {
            quality *= 10;
          }
          return quality;
        }
      }
    } else if (boost::starts_with("1.000", param_value)) {
      // value is a prefix of "1.000"
      return kMaxQuality;
    }
  }
  throw MalformedContentType("Invalid quality value: \'" +
                             std::string(param_value) + '\'');
}

}  // namespace

ContentType::ContentType(std::string_view unparsed) : quality_(kMaxQuality) {
  auto delim_pos = unparsed.find('/');
  if (delim_pos == std::string::npos) {
    throw MalformedContentType("Invalid media type: '" + std::string(unparsed) +
                               '\'');
  }
  type_ = std::string(LtrimOws(unparsed.substr(0, delim_pos)));
  if (type_.empty() ||
      type_.find_first_of(kTypeTokenInvalidChars) != std::string::npos) {
    throw MalformedContentType("Invalid media type: '" + std::string(unparsed) +
                               '\'');
  }
  unparsed.remove_prefix(delim_pos + 1);

  delim_pos = unparsed.find(';');
  subtype_ = std::string(RtrimOws(unparsed.substr(0, delim_pos)));
  if (subtype_.empty() ||
      subtype_.find_first_of(kTypeTokenInvalidChars) != std::string::npos ||
      (type_ == kTokenAny && subtype_ != kTokenAny)) {
    throw MalformedContentType("Invalid media type: \'" + type_ + '/' +
                               std::string(unparsed) + '\'');
  }

  while (delim_pos != std::string::npos) {
    unparsed.remove_prefix(delim_pos + 1);

    auto param_name_end = unparsed.find('=');
    if (param_name_end == std::string::npos) {
      throw MalformedContentType("Malformed parameter in content type");
    }
    auto param_name = LtrimOws(unparsed.substr(0, param_name_end));
    if (param_name.find_first_of(kOwsChars) != std::string_view::npos) {
      throw MalformedContentType("Malformed parameter name in content type: '" +
                                 std::string(param_name) + '\'');
    }
    unparsed.remove_prefix(param_name_end + 1);

    if (unparsed.empty()) {
      throw MalformedContentType("Missing value for parameter: '" +
                                 std::string(param_name) + '\'');
    }
    if (unparsed[0] == '"') {
      throw MalformedContentType("Quoted parameter values are not supported");
    }
    delim_pos = unparsed.find(';');

    if (utils::StrIcaseEqual()(kCharsetParamName, param_name)) {
      charset_ = std::string(RtrimOws(unparsed.substr(0, delim_pos)));
      if (charset_.empty() ||
          charset_.find_first_of(kOwsChars) != std::string::npos) {
        throw MalformedContentType("Invalid charset in content type: '" +
                                   charset_ + '\'');
      }
    } else if (utils::StrIcaseEqual()(kQualityParamName, param_name)) {
      quality_ = ParseQuality(RtrimOws(unparsed.substr(0, delim_pos)));
    }
  }
}

ContentType::ContentType(const std::string& media_range)
    : ContentType(std::string_view{media_range}) {}

ContentType::ContentType(const char* media_range)
    : ContentType(std::string_view{media_range}) {}

std::string ContentType::MediaType() const {
  return fmt::format(FMT_COMPILE("{}/{}"), TypeToken(), SubtypeToken());
}

const std::string& ContentType::TypeToken() const { return type_; }

const std::string& ContentType::SubtypeToken() const { return subtype_; }

bool ContentType::HasExplicitCharset() const { return !charset_.empty(); }

const std::string& ContentType::Charset() const {
  if (HasExplicitCharset()) {
    return charset_;
  }
  return kDefaultCharset;
}

int ContentType::Quality() const { return quality_; }

bool ContentType::DoesAccept(const ContentType& other) const {
  utils::StrIcaseEqual icase_equal{};
  if (TypeToken() != kTokenAny &&
      !icase_equal(TypeToken(), other.TypeToken())) {
    return false;
  }
  if (SubtypeToken() != kTokenAny &&
      !icase_equal(SubtypeToken(), other.SubtypeToken())) {
    return false;
  }
  return icase_equal(Charset(), other.Charset());
}

std::string ContentType::ToString() const {
  fmt::memory_buffer buf;
  fmt::format_to(std::back_inserter(buf), FMT_COMPILE("{}/{}"), TypeToken(),
                 SubtypeToken());
  if (HasExplicitCharset()) {
    fmt::format_to(std::back_inserter(buf), FMT_COMPILE("; {}={}"),
                   kCharsetParamName, Charset());
  }

  // must go after media-range params
  if (Quality() != kMaxQuality) {
    UASSERT(Quality() >= 0);
    UASSERT(Quality() < 1000);
    fmt::format_to(std::back_inserter(buf), FMT_COMPILE("; {}=0.{:03d}"),
                   kQualityParamName, Quality());
  }
  return to_string(buf);
}

bool operator==(const ContentType& lhs, const ContentType& rhs) {
  utils::StrIcaseEqual icase_equal{};
  return icase_equal(lhs.TypeToken(), rhs.TypeToken()) &&
         icase_equal(lhs.SubtypeToken(), rhs.SubtypeToken()) &&
         icase_equal(lhs.Charset(), rhs.Charset()) &&
         lhs.Quality() == rhs.Quality();
}

bool operator!=(const ContentType& lhs, const ContentType& rhs) {
  return !(lhs == rhs);
}

bool operator<(const ContentType& lhs, const ContentType& rhs) {
  utils::StrIcaseCompareThreeWay icase_cmp{};
  // */* has the lowest priority
  if (lhs.TypeToken() == kTokenAny) return rhs.TypeToken() != kTokenAny;
  if (rhs.TypeToken() == kTokenAny) return false;

  // type/* has lower priority than any specific type
  if (lhs.SubtypeToken() == kTokenAny) return rhs.SubtypeToken() != kTokenAny;
  if (rhs.SubtypeToken() == kTokenAny) return false;

  const auto type_token_cmp_result =
      icase_cmp(lhs.TypeToken(), rhs.TypeToken());
  if (type_token_cmp_result == 0) {
    const auto subtype_token_cmp_result =
        icase_cmp(lhs.SubtypeToken(), rhs.SubtypeToken());
    if (subtype_token_cmp_result == 0) {
      // content types with options take precedence
      if (!lhs.HasExplicitCharset()) {
        if (!rhs.HasExplicitCharset()) {
          return lhs.Quality() < rhs.Quality();
        }
        return true;
      }
      if (!rhs.HasExplicitCharset()) return false;

      const auto charset_cmp_result = icase_cmp(lhs.Charset(), rhs.Charset());
      if (charset_cmp_result == 0) {
        return lhs.Quality() < rhs.Quality();
      }
      return charset_cmp_result < 0;
    }
    return subtype_token_cmp_result < 0;
  }
  return type_token_cmp_result < 0;
}

size_t ContentTypeHash::operator()(const ContentType& content_type) const {
  size_t hash = std::hash<int>()(content_type.Quality());
  boost::hash_combine(hash, str_hasher_(content_type.TypeToken()));
  boost::hash_combine(hash, str_hasher_(content_type.SubtypeToken()));
  boost::hash_combine(hash, str_hasher_(content_type.Charset()));
  return hash;
}

std::ostream& operator<<(std::ostream& os, const ContentType& content_type) {
  return os << content_type.ToString();
}

namespace content_type {

const ContentType kApplicationJson = "application/json; charset=utf-8";
const ContentType kTextPlain = "text/plain; charset=utf-8";

}  // namespace content_type
}  // namespace http

USERVER_NAMESPACE_END
