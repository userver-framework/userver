#include <userver/utils/text.hpp>

#include <algorithm>
#include <unordered_map>

#include <boost/algorithm/string.hpp>
#include <boost/locale.hpp>
#include <boost/range/adaptor/transformed.hpp>

#include <userver/engine/shared_mutex.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::text {

std::string Trim(const std::string& str) {
  return boost::algorithm::trim_copy(str);
}

std::string Trim(std::string&& str) {
  boost::algorithm::trim(str);
  return std::move(str);
}

std::vector<std::string> Split(std::string_view str, std::string_view sep) {
  std::vector<std::string> result;
  // FP, m_Size does not change
  // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDeleteLeaks)
  boost::algorithm::split(result, str, boost::algorithm::is_any_of(sep),
                          boost::token_compress_on);
  return result;
}

std::vector<std::string_view> SplitIntoStringViewVector(std::string_view str,
                                                        std::string_view sep) {
  using string_view_split_iterator =
      boost::split_iterator<std::string_view::const_iterator>;

  std::vector<std::string_view> result;
  auto it =
      boost::make_split_iterator(str, boost::token_finder([&sep](const char c) {
                                   return sep.find(c) != std::string_view::npos;
                                 }));
  for (; it != string_view_split_iterator(); ++it) {
    result.emplace_back(it->begin(), it->size());
  }
  return result;
}

std::string Join(const std::vector<std::string>& strs, std::string_view sep) {
  return boost::algorithm::join(strs, sep);
}

namespace {

const std::string kLocaleArabic = "ar";
const std::string kLocaleArabicNumbersLatn = "ar@numbers=latn";

}  // namespace

std::string Format(double value, const std::string& locale, int ndigits,
                   bool is_fixed) {
  std::stringstream res;

  // localization of arabic numerals is broken, fallback to latin (0-9)
  if (locale == kLocaleArabic) {
    // see: https://sites.google.com/site/icuprojectuserguide/locale
    res.imbue(GetLocale(kLocaleArabicNumbersLatn));
  } else {
    res.imbue(GetLocale(locale));
  }

  if (is_fixed) res.setf(std::ios::fixed, std::ios::floatfield);
  res.precision(ndigits);

  res << boost::locale::as::number << value;
  return res.str();
}

std::string Format(boost::multiprecision::cpp_dec_float_50 value, int ndigits) {
  std::stringstream res;
  res << std::setprecision(ndigits) << value;
  return res.str();
}

std::string Format(double value, int ndigits) {
  std::stringstream res;
  res << std::setprecision(ndigits) << value;
  return res.str();
}

bool StartsWith(std::string_view hay, std::string_view needle) {
  return hay.substr(0, needle.size()) == needle;
}

bool EndsWith(std::string_view hay, std::string_view needle) {
  return hay.size() >= needle.size() &&
         hay.substr(hay.size() - needle.size()) == needle;
}

std::string ToLower(std::string_view str, const std::string& locale) {
  return boost::locale::to_lower(str.data(), str.data() + str.size(),
                                 GetLocale(locale));
}

std::string Capitalize(std::string_view str, const std::string& locale) {
  return boost::locale::to_title(str.data(), str.data() + str.size(),
                                 GetLocale(locale));
}

std::string RemoveQuotes(std::string_view str) {
  if (str.empty()) return {};
  if (str.front() != '"' || str.back() != '"') return std::string{str};
  return std::string{str.substr(1, str.size() - 2)};
}

bool IsAscii(char ch) noexcept { return static_cast<unsigned char>(ch) <= 127; }

bool IsAsciiSpace(char ch) noexcept {
  switch (ch) {
    case ' ':
    case '\f':
    case '\n':
    case '\r':
    case '\t':
    case '\v':
      return true;
    default:;
  }
  return false;
}

bool IsAscii(std::string_view text) noexcept {
  return std::all_of(text.cbegin(), text.cend(),
                     [](auto ch) { return IsAscii(ch); });
}

const std::locale& GetLocale(const std::string& name) {
  static engine::SharedMutex m;
  using locales_map_t = std::unordered_map<std::string, std::locale>;
  static locales_map_t locales;
  {
    std::shared_lock read_lock(m);
    auto it = static_cast<const locales_map_t&>(locales).find(name);
    if (it != locales.cend()) {
      return it->second;
    }
  }

  boost::locale::generator gen;
  std::locale loc = gen(name);
  {
    std::unique_lock write_lock(m);
    return locales.emplace(name, std::move(loc)).first->second;
  }
}

namespace utf8 {

namespace {
// http://www.unicode.org/versions/latest/
//
// Unicode Encoding Forms
// Table 3-7. Well-Formed UTF-8 Byte Sequences
struct InclusiveRange {
  unsigned char from;
  unsigned char to;
};

constexpr InclusiveRange kWellFormed1Bytes[][1] = {
    {{0x00, 0x7f}},
};

constexpr InclusiveRange kWellFormed2Bytes[][2] = {
    {{0xc2, 0xdf}, {0x80, 0xbf}},
};

constexpr InclusiveRange kWellFormed3Bytes[][3] = {
    {{0xe0, 0xe0}, {0xa0, 0xbf}, {0x80, 0xbf}},
    {{0xe1, 0xec}, {0x80, 0xbf}, {0x80, 0xbf}},
    {{0xed, 0xed}, {0x80, 0x9f}, {0x80, 0xbf}},
    {{0xee, 0xef}, {0x80, 0xbf}, {0x80, 0xbf}},
};

constexpr InclusiveRange kWellFormed4Bytes[][4] = {
    {{0xf0, 0xf0}, {0x90, 0xbf}, {0x80, 0xbf}, {0x80, 0xbf}},
    {{0xf1, 0xf3}, {0x80, 0xbf}, {0x80, 0xbf}, {0x80, 0xbf}},
    {{0xf4, 0xf4}, {0x80, 0x8f}, {0x80, 0xbf}, {0x80, 0xbf}},
};

constexpr unsigned char kMinFirstByteValueFor2ByteCodePoint =
    kWellFormed2Bytes[0][0].from;
constexpr unsigned char kMinFirstByteValueFor3ByteCodePoint =
    kWellFormed3Bytes[0][0].from;
constexpr unsigned char kMinFirstByteValueFor4ByteCodePoint =
    kWellFormed4Bytes[0][0].from;

template <std::size_t Rows, std::size_t Columns>
bool IsValidBytes(const unsigned char* bytes,
                  const InclusiveRange (&range)[Rows][Columns]) noexcept {
  for (std::size_t row = 0; row < Rows; ++row) {
    bool valid = true;
    for (std::size_t column = 0; column < Columns; ++column) {
      const InclusiveRange table_value = range[row][column];
      const auto byte = bytes[column];

      if (table_value.from > byte || table_value.to < byte) {
        valid = false;
        break;
      }
    }

    if (valid) {
      return true;
    }
  }

  return false;
}

size_t FindTruncatedEndingCorrectSize(std::string_view str) {
  if (str.empty()) {
    return str.size();
  }

  // check if `str` ends with 1-byte character
  if (!(str.back() & 0x80)) {
    return str.size();
  }

  // no symbol with a length > 4 => no proper prefix with a length > 3
  size_t max_suffix_len = std::min(str.size(), std::size_t{3});
  for (size_t suffix_len = 1; suffix_len <= max_suffix_len; ++suffix_len) {
    size_t pos = str.size() - suffix_len;

    // first byte of a multibyte character
    if ((str[pos] & 0xc0) == 0xc0) {
      size_t expected_len = utf8::CodePointLengthByFirstByte(str[pos]);
      // check if current suffix is a proper prefix of some multibyte character
      if (expected_len > suffix_len) {
        return str.size() - suffix_len;
      } else {  // else character is not truncated or `str` was not in utf-8
        return str.size();
      }
    }

    // Check if current byte is a continuation byte of a multibyte character.
    // If not then `str` is not a truncated utf-8 string.
    if ((str[pos] & 0xc0) != 0x80) {
      return str.size();
    }
  }

  return str.size();
}

}  // namespace

unsigned CodePointLengthByFirstByte(unsigned char c) noexcept {
  if (c < kMinFirstByteValueFor2ByteCodePoint) {
    return 1;
  } else if (c < kMinFirstByteValueFor3ByteCodePoint) {
    return 2;
  } else if (c < kMinFirstByteValueFor4ByteCodePoint) {
    return 3;
  }

  return 4;
}

bool IsWellFormedCodePoint(const unsigned char* bytes,
                           std::size_t length) noexcept {
  UASSERT(bytes);
  UASSERT(length != 0);

  const auto code_point_length = CodePointLengthByFirstByte(bytes[0]);
  if (length < code_point_length) {
    return false;
  }

  switch (code_point_length) {
    case 1:
      return IsValidBytes(bytes, utf8::kWellFormed1Bytes);
    case 2:
      return IsValidBytes(bytes, utf8::kWellFormed2Bytes);
    case 3:
      return IsValidBytes(bytes, utf8::kWellFormed3Bytes);
    case 4:
      return IsValidBytes(bytes, utf8::kWellFormed4Bytes);
    default:
      UASSERT_MSG(false, "Invalid UTF-8 code point length");
  }

  return false;
}

bool IsValid(const unsigned char* bytes, std::size_t length) noexcept {
  for (size_t i = 0; i < length; ++i) {
    if (!IsWellFormedCodePoint(bytes + i, length - i)) {
      return false;
    } else {
      i += CodePointLengthByFirstByte(bytes[i]) - 1;
    }
  }

  return true;
}

std::size_t GetCodePointsCount(std::string_view text) {
  const auto* const bytes = reinterpret_cast<const unsigned char*>(text.data());
  const auto size = text.size();
  std::size_t count = 0;
  for (std::size_t i = 0; i < size; i += CodePointLengthByFirstByte(bytes[i])) {
    if (!IsWellFormedCodePoint(bytes + i, size - i)) {
      throw std::runtime_error("GetCodePointsCount: text is not in utf-8");
    }
    count++;
  }
  return count;
}

void TrimTruncatedEnding(std::string& str) {
  size_t correct_size = FindTruncatedEndingCorrectSize(std::string_view{str});
  UASSERT_MSG(correct_size <= str.size(),
              "Function FindTruncatedEndingCorrectSize is broken: "
              "'correct_size' cannot be greater than 'str'");

  if (correct_size < str.size()) {
    str.resize(correct_size);
  }
}

void TrimViewTruncatedEnding(std::string_view& view) {
  size_t correct_size = FindTruncatedEndingCorrectSize(view);
  UASSERT_MSG(correct_size <= view.size(),
              "Function FindTruncatedEndingCorrectSize is broken: "
              "'correct_size' cannot be greater than 'view'");

  if (correct_size < view.size()) {
    view = std::string_view{view.data(), correct_size};
  }
}

std::size_t GetTextPosByCodePointPos(std::string_view text,
                                     std::size_t pos) noexcept {
  size_t text_pos = 0;
  while (text_pos < text.size() && pos > 0) {
    text_pos += CodePointLengthByFirstByte(text[text_pos]);
    pos--;
  }
  return std::min(text_pos, text.length());
}

void RemovePrefix(std::string& text, std::size_t count) noexcept {
  text.erase(0, GetTextPosByCodePointPos(text, count));
}

void RemoveViewPrefix(std::string_view& text, std::size_t count) noexcept {
  text.remove_prefix(GetTextPosByCodePointPos(text, count));
}

void TakePrefix(std::string& text, std::size_t count) noexcept {
  text.erase(GetTextPosByCodePointPos(text, count));
}

void TakeViewPrefix(std::string_view& text, std::size_t count) noexcept {
  text = text.substr(0, GetTextPosByCodePointPos(text, count));
}

}  // namespace utf8

bool IsUtf8(std::string_view text) noexcept {
  const auto* const bytes = reinterpret_cast<const unsigned char*>(text.data());
  const auto size = text.size();

  return utf8::IsValid(bytes, size);
}

bool IsPrintable(std::string_view text, bool ascii_only) noexcept {
  if (ascii_only) {
    return std::all_of(text.cbegin(), text.cend(),
                       [](int c) { return (c >= 32 && c <= 126); });
  }

  return IsUtf8(text) && !std::any_of(text.cbegin(), text.cend(), [](int c) {
           return ((c >= 0 && c <= 31) || c == 127);
         });
}

bool IsCString(std::string_view text) noexcept {
  return text.find('\0') == std::string::npos;
}

std::string CamelCaseToSnake(std::string_view camel) {
  std::string snake;

  if (!camel.empty()) {
    snake += static_cast<char>(std::tolower(camel[0]));
    for (size_t i = 1; i < camel.size(); ++i) {
      if (std::isupper(camel[i])) {
        snake += '_';
        snake += static_cast<char>(std::tolower(camel[i]));
      } else {
        snake += camel[i];
      }
    }
  }

  return snake;
}

}  // namespace utils::text

USERVER_NAMESPACE_END
